/*
 * Chat Application - GTK GUI Client
 * For CentOS 6 32-bit with kernel crypto module
 * Compile: gcc -o chat_client chat_client.c `pkg-config --cflags --libs gtk+-2.0` -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "../include/crypto_module.h"
#include "../include/chat_protocol.h"

#define SERVER_HOST "localhost"
#define SERVER_PORT 5555

/* GUI State */
typedef struct {
    GtkWidget *main_window;
    GtkWidget *login_window;
    GtkWidget *chat_window;
    GtkWidget *message_view;
    GtkWidget *message_input;
    GtkWidget *contact_list;
    GtkWidget *status_bar;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *receiver_combo;

    uint32_t user_id;
    char username[MAX_USERNAME_LEN];
    char server_host[256];
    int server_port;
    int socket_fd;
    int crypto_device;
    int connected;
    pthread_t receive_thread;
    GtklistStore *contact_store;
    int running;
} app_state_t;

static app_state_t app_state;

/* SHA1 via kernel module */
static void compute_sha1(const unsigned char *input, unsigned long len,
                        unsigned char *output)
{
    struct sha1_request req;

    if (app_state.crypto_device < 0) {
        fprintf(stderr, "ERROR: Crypto device not open\n");
        return;
    }

    if (len > SHA1_MAX_INPUT) {
        fprintf(stderr, "ERROR: Input too long for SHA1\n");
        return;
    }

    memcpy(req.input, input, len);
    req.input_len = len;

    if (ioctl(app_state.crypto_device, CRYPTO_IOCTL_SHA1_HASH, &req) < 0) {
        fprintf(stderr, "ERROR: SHA1 ioctl failed\n");
        return;
    }

    memcpy(output, req.digest, SHA1_DIGEST_SIZE);
}

/* DES encrypt via kernel module */
static int des_encrypt(const unsigned char *key, const unsigned char *input,
                      unsigned long input_len, unsigned char *output,
                      unsigned long *output_len)
{
    struct des_request req;
    unsigned long padded_len;
    int i, pad_len;

    if (app_state.crypto_device < 0) {
        return -1;
    }

    /* Pad input to 8-byte boundary */
    padded_len = (input_len + 7) & ~7;
    pad_len = padded_len - input_len;

    if (padded_len > MAX_CRYPTO_DATA) {
        return -1;
    }

    memcpy(req.input, input, input_len);
    for (i = input_len; i < padded_len; i++) {
        req.input[i] = pad_len;
    }

    memcpy(req.key, key, DES_KEY_SIZE);
    req.input_len = padded_len;
    req.mode = 0; /* Encrypt */

    if (ioctl(app_state.crypto_device, CRYPTO_IOCTL_DES_ENCRYPT, &req) < 0) {
        return -1;
    }

    *output_len = req.output_len;
    memcpy(output, req.output, req.output_len);

    return 0;
}

/* Update status bar */
static void update_status(const char *fmt, ...)
{
    va_list args;
    char buffer[512];

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (app_state.status_bar) {
        gtk_statusbar_push(GTK_STATUSBAR(app_state.status_bar), 0, buffer);
    }

    fprintf(stdout, "[Status] %s\n", buffer);
}

/* Add message to message view */
static void add_message(const char *sender, const char *text, gboolean is_own)
{
    GtkTextBuffer *buffer;
    GtkTextIter end;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[32];
    char full_msg[MAX_MESSAGE_LEN + 128];

    snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d",
             tm->tm_hour, tm->tm_min, tm->tm_sec);

    snprintf(full_msg, sizeof(full_msg),
             "[%s] %s: %s\n",
             timestamp, sender, text);

    if (!app_state.message_view) {
        return;
    }

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_state.message_view));
    gtk_text_buffer_get_end_iter(buffer, &end);

    if (is_own) {
        gtk_text_buffer_insert_with_tags_by_name(buffer, &end, full_msg, -1,
                                                  "own_message", NULL);
    } else {
        gtk_text_buffer_insert_with_tags_by_name(buffer, &end, full_msg, -1,
                                                  "other_message", NULL);
    }

    /* Scroll to bottom */
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(app_state.message_view), &end, 0, FALSE, 0, 0);
}

/* Receive messages from server */
static void* receive_thread_func(void *arg)
{
    unsigned char buffer[4096];
    int bytes_read;
    packet_header_t *header;
    message_pkt_t *msg;

    update_status("Receive thread started");

    while (app_state.running && app_state.connected) {
        bytes_read = recv(app_state.socket_fd, buffer, sizeof(buffer), 0);

        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                perror("recv");
            }
            app_state.connected = 0;
            update_status("Disconnected from server");
            break;
        }

        if (bytes_read < sizeof(packet_header_t)) {
            continue;
        }

        header = (packet_header_t *)buffer;

        switch (header->type) {
            case PKT_MESSAGE:
                msg = (message_pkt_t *)buffer;
                add_message("Other User", (char *)msg->encrypted_body, FALSE);
                update_status("Message received from user %u", header->sender_id);
                break;

            case PKT_AUTH_RESPONSE: {
                auth_resp_t *auth = (auth_resp_t *)buffer;
                if (auth->status == 0) {
                    update_status("Authentication successful!");
                    app_state.user_id = auth->user_id;
                } else {
                    update_status("Authentication failed");
                    app_state.connected = 0;
                }
                break;
            }

            case PKT_HEARTBEAT:
                /* Respond to heartbeat */
                send(app_state.socket_fd, buffer, sizeof(packet_header_t), 0);
                break;

            case PKT_ERROR:
                update_status("Server error");
                break;

            default:
                fprintf(stderr, "Unknown packet type: %d\n", header->type);
                break;
        }
    }

    update_status("Receive thread exiting");
    return NULL;
}

/* Send message callback */
static void on_send_message(GtkWidget *widget, gpointer data)
{
    const char *message_text;
    GtkComboBox *combo;
    int receiver_id = 0;
    message_pkt_t msg;
    unsigned char encrypted[MAX_MESSAGE_LEN + DES_BLOCK_SIZE];
    unsigned long encrypted_len;
    char *combo_text;

    if (!app_state.connected) {
        update_status("Not connected to server");
        return;
    }

    message_text = gtk_entry_get_text(GTK_ENTRY(app_state.message_input));

    if (!message_text || strlen(message_text) == 0) {
        update_status("Message is empty");
        return;
    }

    /* Get receiver ID from combo box */
    combo = GTK_COMBO_BOX(app_state.receiver_combo);
    combo_text = gtk_combo_box_get_active_text(combo);

    if (!combo_text) {
        update_status("Please select a receiver");
        return;
    }

    receiver_id = atoi(combo_text);
    if (receiver_id <= 0) {
        update_status("Invalid receiver");
        return;
    }

    /* Encrypt message with DES */
    if (des_encrypt((unsigned char *)"\x01\x23\x45\x67\x89\xAB\xCD\xEF",
                   (unsigned char *)message_text,
                   strlen(message_text),
                   encrypted,
                   &encrypted_len) != 0) {
        update_status("Encryption failed");
        return;
    }

    /* Build message packet */
    msg.header.type = PKT_MESSAGE;
    msg.header.sender_id = app_state.user_id;
    msg.header.receiver_id = receiver_id;
    msg.header.sequence = 0;
    msg.header.timestamp = time(NULL);
    msg.header.length = encrypted_len;
    memcpy(msg.iv, "\x00\x01\x02\x03\x04\x05\x06\x07", DES_BLOCK_SIZE);
    memcpy(msg.encrypted_body, encrypted, encrypted_len);

    /* Send to server */
    if (send(app_state.socket_fd, &msg,
            sizeof(packet_header_t) + encrypted_len, 0) < 0) {
        perror("send");
        update_status("Failed to send message");
        return;
    }

    add_message(app_state.username, message_text, TRUE);
    gtk_entry_set_text(GTK_ENTRY(app_state.message_input), "");
    update_status("Message sent");

    g_free(combo_text);
}

/* Connect to server */
static int connect_to_server(const char *username, const char *password, int is_register)
{
    struct sockaddr_in server_addr;
    unsigned char password_hash[SHA1_DIGEST_SIZE];
    login_pkt_t login_pkt;
    register_pkt_t reg_pkt;
    int bytes_read;
    unsigned char buffer[sizeof(auth_resp_t)];
    auth_resp_t *auth_resp;

    /* Close existing connection */
    if (app_state.socket_fd > 0) {
        close(app_state.socket_fd);
    }

    /* Create socket */
    app_state.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (app_state.socket_fd < 0) {
        perror("socket");
        return -1;
    }

    /* Connect to server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_HOST);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(app_state.socket_fd, (struct sockaddr *)&server_addr,
               sizeof(server_addr)) < 0) {
        perror("connect");
        close(app_state.socket_fd);
        app_state.socket_fd = -1;
        return -1;
    }

    /* Compute password hash */
    compute_sha1((unsigned char *)password, strlen(password), password_hash);

    /* Send authentication packet */
    if (is_register) {
        reg_pkt.header.type = PKT_REGISTER;
        reg_pkt.header.sender_id = 0;
        reg_pkt.header.sequence = 1;
        reg_pkt.header.timestamp = time(NULL);
        reg_pkt.header.length = 0;
        strncpy(reg_pkt.username, username, MAX_USERNAME_LEN - 1);
        memcpy(reg_pkt.password_hash, password_hash, sizeof(password_hash));

        if (send(app_state.socket_fd, &reg_pkt, sizeof(reg_pkt), 0) < 0) {
            perror("send registration");
            close(app_state.socket_fd);
            return -1;
        }
    } else {
        login_pkt.header.type = PKT_LOGIN;
        login_pkt.header.sender_id = 0;
        login_pkt.header.sequence = 1;
        login_pkt.header.timestamp = time(NULL);
        login_pkt.header.length = 0;
        strncpy(login_pkt.username, username, MAX_USERNAME_LEN - 1);
        memcpy(login_pkt.password_hash, password_hash, sizeof(password_hash));

        if (send(app_state.socket_fd, &login_pkt, sizeof(login_pkt), 0) < 0) {
            perror("send login");
            close(app_state.socket_fd);
            return -1;
        }
    }

    /* Receive auth response */
    bytes_read = recv(app_state.socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_read < sizeof(auth_resp_t)) {
        update_status("Invalid auth response");
        close(app_state.socket_fd);
        return -1;
    }

    auth_resp = (auth_resp_t *)buffer;
    if (auth_resp->status != 0) {
        update_status("Authentication failed");
        close(app_state.socket_fd);
        return -1;
    }

    app_state.user_id = auth_resp->user_id;
    strncpy(app_state.username, username, MAX_USERNAME_LEN - 1);
    app_state.connected = 1;

    /* Start receive thread */
    if (pthread_create(&app_state.receive_thread, NULL, receive_thread_func, NULL) != 0) {
        update_status("Failed to create receive thread");
        close(app_state.socket_fd);
        return -1;
    }

    return 0;
}

/* Login button callback */
static void on_login_clicked(GtkWidget *widget, gpointer data)
{
    const char *username, *password;
    int is_register = GPOINTER_TO_INT(data);

    username = gtk_entry_get_text(GTK_ENTRY(app_state.username_entry));
    password = gtk_entry_get_text(GTK_ENTRY(app_state.password_entry));

    if (!username || !password || strlen(username) == 0 || strlen(password) == 0) {
        gtk_label_set_text(GTK_LABEL(app_state.status_bar),
                          "Please enter username and password");
        return;
    }

    update_status("Connecting to %s:%d...", SERVER_HOST, SERVER_PORT);

    if (connect_to_server(username, password, is_register) == 0) {
        update_status("Connected and authenticated!");
        gtk_widget_hide(app_state.login_window);
        gtk_widget_show(app_state.chat_window);
    } else {
        update_status("Connection or authentication failed");
    }
}

/* Create login window */
static GtkWidget* create_login_window(void)
{
    GtkWidget *window;
    GtkWidget *vbox, *hbox;
    GtkWidget *label, *button;
    GtkWidget *frame;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Application - Login");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 250);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Title */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<big><b>Chat Application</b></big>");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* Username */
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Username:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    app_state.username_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), app_state.username_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Password */
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Password:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    app_state.password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app_state.password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), app_state.password_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Buttons */
    hbox = gtk_hbox_new(TRUE, 5);
    button = gtk_button_new_with_label("Login");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_login_clicked),
                     GINT_TO_POINTER(0));
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Register");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_login_clicked),
                     GINT_TO_POINTER(1));
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Status bar */
    app_state.status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), app_state.status_bar, FALSE, FALSE, 0);

    gtk_widget_show_all(window);

    return window;
}

/* Create chat window */
static GtkWidget* create_chat_window(void)
{
    GtkWidget *window;
    GtkWidget *vbox, *hbox;
    GtkWidget *label, *button;
    GtkWidget *scrolled_window;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Messages view */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    app_state.message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app_state.message_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app_state.message_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled_window), app_state.message_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    /* Receiver selection */
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Send to:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    app_state.receiver_combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(app_state.receiver_combo), "User 2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(app_state.receiver_combo), "User 3");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app_state.receiver_combo), 0);
    gtk_box_pack_start(GTK_BOX(hbox), app_state.receiver_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Message input */
    hbox = gtk_hbox_new(FALSE, 5);
    app_state.message_input = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), app_state.message_input, TRUE, TRUE, 0);
    button = gtk_button_new_with_label("Send");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_send_message), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    return window;
}

/* Main */
int main(int argc, char *argv[])
{
    GtkTextBuffer *buffer;
    GtkTextTag *tag;

    gtk_init(&argc, &argv);

    /* Initialize crypto device */
    app_state.crypto_device = open("/dev/chat_crypto", O_RDWR);
    if (app_state.crypto_device < 0) {
        fprintf(stderr, "ERROR: Failed to open /dev/chat_crypto\n");
        fprintf(stderr, "Make sure kernel module is loaded.\n");
        return 1;
    }

    /* Initialize state */
    app_state.socket_fd = -1;
    app_state.connected = 0;
    app_state.running = 1;
    strncpy(app_state.server_host, SERVER_HOST, sizeof(app_state.server_host));
    app_state.server_port = SERVER_PORT;

    /* Create windows */
    app_state.login_window = create_login_window();
    app_state.chat_window = create_chat_window();

    /* Setup text tags for message view */
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_state.message_view));
    
    tag = gtk_text_buffer_create_tag(buffer, "own_message",
                                    "foreground", "blue", NULL);
    tag = gtk_text_buffer_create_tag(buffer, "other_message",
                                    "foreground", "green", NULL);

    gtk_main();

    if (app_state.socket_fd > 0) {
        close(app_state.socket_fd);
    }
    close(app_state.crypto_device);

    return 0;
}

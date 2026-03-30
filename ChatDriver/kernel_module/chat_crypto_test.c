#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define CRYPTO_IOCTL_SHA1    _IOWR('c', 1, struct crypto_req)
#define CRYPTO_IOCTL_DES_ENC _IOWR('c', 2, struct crypto_req)
#define CRYPTO_IOCTL_DES_DEC _IOWR('c', 3, struct crypto_req)

struct crypto_req {
    char data[256];
    int data_len;
    char key[8];
    char result[256];
    int result_len;
};

int main(int argc, char *argv[]) {
    int i;
    if (argc < 3) return -1;

    int fd = open("/dev/kma_crypto", O_RDWR);
    if (fd < 0) { perror("Khong the mo /dev/kma_crypto"); return -1; }

    struct crypto_req req;
    memset(&req, 0, sizeof(req));

    strncpy(req.data, argv[2], 255);
    req.data_len = strlen(req.data);

    if (strcmp(argv[1], "sha1") == 0) {
        ioctl(fd, CRYPTO_IOCTL_SHA1, &req);
        for (i = 0; i < req.result_len; i++) printf("%02x", (unsigned char)req.result[i]);
        printf("\n");
    }
    else if (strcmp(argv[1], "enc") == 0) {
        strncpy(req.key, argv[3], 8);
        ioctl(fd, CRYPTO_IOCTL_DES_ENC, &req);
        for (i = 0; i < req.result_len; i++) printf("%02x", (unsigned char)req.result[i]);
        printf("\n");
    }
    else if (strcmp(argv[1], "dec") == 0) {
        strncpy(req.key, argv[3], 8);
        int hex_len = strlen(argv[2]) / 2;
        req.data_len = hex_len;
        for (i = 0; i < hex_len; i++) {
            unsigned int byte;
            sscanf(&argv[2][i*2], "%2x", &byte);
            req.data[i] = (char)byte;
        }
        ioctl(fd, CRYPTO_IOCTL_DES_DEC, &req);
        printf("%s\n", req.result);
    }

    close(fd);
    return 0;
}
# Installation & Deployment Guide - CentOS 6 32-bit

## System Setup

### 1. Load Kernel Module

```bash
# Navigate to kernel module directory
cd ChatDriver/kernel_module

# Make and install
sudo make install

# Verify module is loaded
lsmod | grep chat_crypto
# Output: chat_crypto X X -

# Check device was created
ls -l /dev/chat_crypto
# Output: crw-rw-rw- ... /dev/chat_crypto
```

If module fails to load, check dmesg:
```bash
sudo dmesg | tail -20
```

### 2. Set Up Daemon Service

```bash
# Copy daemon executable to system location
sudo cp userspace_service/chat_daemon /usr/local/bin/

# Create log directory
sudo mkdir -p /var/log/chat/
sudo chmod 755 /var/log/chat/

# Create systemd/init service script (choose one based on CentOS 6 setup)
```

#### Option A: SysVinit Script (Traditional CentOS 6)

Create `/etc/init.d/chat_daemon`:

```bash
#!/bin/bash
#
# chat_daemon       Chat Application Daemon
#
# chkconfig: 2345 99 1
# description: Chat application server with DES encryption
#

### BEGIN INIT INFO
# Provides:       chat_daemon
# Required-Start: $network
# Required-Stop:  $network
# Default-Start:  2 3 4 5
# Default-Stop:   0 1 6
# Short-Description: Chat Application Daemon
### END INIT INFO

DAEMON=/usr/local/bin/chat_daemon
PIDFILE=/var/run/chat_daemon.pid

. /etc/rc.d/init.d/functions

start() {
    echo -n "Starting chat_daemon: "
    
    # Ensure kernel module is loaded
    lsmod | grep -q chat_crypto
    if [ $? -ne 0 ]; then
        echo "ERROR: chat_crypto kernel module not loaded"
        echo "Run: insmod /path/to/chat_crypto.ko"
        failure
        return 1
    fi
    
    # Start the daemon
    $DAEMON &
    pid=$!
    echo $pid > $PIDFILE
    
    if [ $? -eq 0 ]; then
        success
        echo
        return 0
    else
        failure
        echo
        return 1
    fi
}

stop() {
    echo -n "Stopping chat_daemon: "
    
    if [ -f $PIDFILE ]; then
        pid=$(cat $PIDFILE)
        kill $pid 2>/dev/null
        rm $PIDFILE
        success
        echo
    else
        echo "daemon not running"
    fi
}

status() {
    if [ -f $PIDFILE ]; then
        pid=$(cat $PIDFILE)
        ps -p $pid > /dev/null
        if [ $? -eq 0 ]; then
            echo "chat_daemon is running (PID: $pid)"
            return 0
        fi
    fi
    echo "chat_daemon is not running"
    return 1
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        start
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: {start|stop|restart|status}"
        exit 1
esac

exit $?
```

Make it executable and register:
```bash
sudo chmod +x /etc/init.d/chat_daemon
sudo chkconfig --add chat_daemon
sudo chkconfig chat_daemon on
```

### 3. Start the Service

```bash
# Method 1: Using service command
sudo service chat_daemon start

# Method 2: Using chkconfig
sudo systemctl start chat_daemon  # (if systemd available)

# Verify it's running
sudo service chat_daemon status

# Check logs
tail -f /var/log/chat_daemon.log
```

## Client Setup

### 1. Install GTK GUI Client

```bash
# Copy client executable
sudo cp gtk_gui/chat_client /usr/local/bin/

# Create desktop shortcut (optional)
cat > ~/.local/share/applications/chatty.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=Chat Application
Exec=/usr/local/bin/chat_client
Icon=dialog-messages
Categories=Network;Communication;
EOF
```

### 2. Run Client

```bash
# From terminal
chat_client

# Or from applications menu (if desktop entry created)
```

## Network Configuration

### 1. Firewall Setup

```bash
# If using iptables firewall
sudo iptables -A INPUT -p tcp --dport 5555 -j ACCEPT
sudo service iptables save
sudo service iptables restart

# If using firewalld (newer systems)
sudo firewall-cmd --permanent --add-port=5555/tcp
sudo firewall-cmd --reload
```

### 2. Configure Server Address

In `gtk_gui/chat_client.c`, modify:
```c
#define SERVER_HOST "localhost"  // Change to server IP
#define SERVER_PORT 5555         // Change if needed
```

Rebuild and reinstall.

## Running Multiple Instances

### Terminal 1: Start Daemon

```bash
sudo service chat_daemon start
# or
/usr/local/bin/chat_daemon
```

### Terminal 2: Run First Client

```bash
/usr/local/bin/chat_client
```

### Terminal 3: Run Second Client

```bash
/usr/local/bin/chat_client
```

## Testing the Application

### 1. Instance 1 - Register

```
Username: alice
Password: password123
Click: Register
```

### 2. Instance 2 - Register

```
Username: bob
Password: pass456
Click: Register
```

### 3. Instance 1 - Login

```
Username: alice
Password: password123
Click: Login
Status: "Connected and authenticated!"
```

### 4. Instance 2 - Login

```
Username: bob
Password: pass456
Click: Login
Status: "Connected and authenticated!"
```

### 5. Send Message

In Instance 1:
- Select "User 2" from dropdown
- Type: "Hello Bob!"
- Click Send
- Message appears in blue (own message)

In Instance 2:
- Message from Bob appears in green (other message)
- Type: "Hi Alice!"
- Click Send

## Troubleshooting

### Kernel Module Load Fails

```bash
# Check dmesg for errors
sudo dmesg | grep -i chat

# Verify kernel headers are installed
ls /lib/modules/$(uname -r)/build/

# Rebuild kernel module
cd kernel_module
make clean
make
sudo make install
```

### Daemon Won't Start

```bash
# Run manually to see errors
/usr/local/bin/chat_daemon

# Check /dev/chat_crypto exists
ls -l /dev/chat_crypto

# Check permissions
sudo chmod 666 /dev/chat_crypto
```

### Client Connection Fails

```bash
# Verify daemon is running
ps aux | grep chat_daemon

# Check if port is listening
netstat -tlnp | grep 5555

# Verify firewall
iptables -L -n | grep 5555
```

### Recompile Everything

```bash
# Clean builds
cd kernel_module && make clean
cd ../userspace_service && rm -f chat_daemon

# Rebuild all
cd .. && make -f Makefile.master clean all
```

## Performance Tuning (Optional)

### Increase Max Clients

In `userspace_service/chat_daemon.c`:
```c
#define MAX_CLIENTS 100  // Increase as needed
```

### Increase Socket Backlog

In `userspace_service/chat_daemon.c`:
```c
if (listen(server_socket, 5) < 0) {  // Change 5 to higher number
```

### Enable SO_KEEPALIVE

Add to `userspace_service/chat_daemon.c` after socket creation:
```c
int keepalive = 1;
setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
```

## Uninstallation

```bash
# Stop service
sudo service chat_daemon stop

# Remove from autostart
sudo chkconfig chat_daemon off
sudo chkconfig --del chat_daemon

# Remove files
sudo rm /etc/init.d/chat_daemon
sudo rm /usr/local/bin/chat_daemon
sudo rm /usr/local/bin/chat_client
sudo rm -rf /var/log/chat/

# Unload kernel module
sudo rmmod chat_crypto
```

## Next Steps

See README.md for architecture overview and features.

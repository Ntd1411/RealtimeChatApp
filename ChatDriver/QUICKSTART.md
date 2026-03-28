# Quick Start Guide - Chat Driver Application

## 📦 Prerequisites

```bash
# Install dependencies on CentOS 6
sudo yum groupinstall -y "Development Tools"
sudo yum install -y kernel-devel-$(uname -r)
sudo yum install -y kernel-headers-$(uname -r)
sudo yum install -y gtk2-devel glib2-devel
```

## 🏗️ Build Everything

```bash
cd ChatDriver

# Make build script executable
chmod +x build.sh

# Build all components
./build.sh all
```

Expected output:
```
[*] Checking dependencies...
[*] Dependencies OK
[*] Building kernel module...
[*] Kernel module built successfully
[*] Building userspace daemon...
[*] Daemon built successfully
[*] Building GTK client...
[*] Client built successfully
[*] Copying artifacts to ./build...
```

## ⚡ Quick Install & Run

### Terminal 1: Load Module & Start Daemon

```bash
cd ChatDriver/kernel_module
sudo make install

# Check module loaded
lsmod | grep chat_crypto

# Start daemon
sudo ChatDriver/build/chat_daemon &
```

### Terminal 2: Run First Client

```bash
ChatDriver/build/chat_client
```

**Register/Login:**
- Username: `alice`
- Password: `password123`
- Click: `Register` → then `Login`

### Terminal 3: Run Second Client

```bash
ChatDriver/build/chat_client
```

**Register/Login:**
- Username: `bob`
- Password: `pass456`
- Click: `Register` → then `Login`

### Send Messages

**In Alice's client:**
- Select "User 2" from dropdown
- Type: "Hello Bob!"
- Click: `Send`

**In Bob's client:**
- Message appears: `[12:34:56] Other User: Hello Bob!`
- Type response: "Hi Alice!"
- Click: `Send`

## ✅ Verification

Check that everything works:

```bash
# Kernel module loaded?
lsmod | grep chat_crypto
# OUTPUT: chat_crypto    12345  0

# Device created?
ls -l /dev/chat_crypto
# OUTPUT: crw-rw-rw- ... /dev/chat_crypto

# Daemon running?
ps aux | grep chat_daemon
# OUTPUT: /usr/local/bin/chat_daemon

# Port listening?
netstat -tlnp | grep 5555
# OUTPUT: tcp  0  0 0.0.0.0:5555  0.0.0.0:*  LISTEN  pid/chat_daemon
```

## 🔍 Troubleshooting

### Module won't load
```bash
# Rebuild module
cd ChatDriver/kernel_module
make clean && make
sudo make install
```

### Daemon crashes on start
```bash
# Run manually to see errors
/usr/local/bin/chat_daemon

# Check device exists
ls -l /dev/chat_crypto

# If missing, reload module
sudo rmmod chat_crypto
cd kernel_module && sudo make install
```

### Client can't connect
```bash
# Is daemon running?
ps aux | grep chat_daemon

# Is port open?
netstat -tlnp | grep 5555

# Check firewall
sudo iptables -L -n | grep 5555
```

## 📚 Full Documentation

- **README.md** - Project overview and features
- **BUILD.md** - Detailed build instructions
- **INSTALL.md** - Deployment and operation guide
- **include/chat_protocol.h** - Protocol specification
- **include/crypto_module.h** - Crypto API reference

## 🎯 Architecture Summary

```
CLIENT 1 (GTK GUI)              CLIENT 2 (GTK GUI)
       │                              │
       └──────────────┬───────────────┘
                      │
              Socket (Port 5555)
                      │
              CHAT_DAEMON SERVICE
           (Relay, Auth, User DB)
                      │
                ioctl() Calls
                      │
          KERNEL MODULE (/dev/chat_crypto)
         (DES Encryption, SHA1 Hashing)
```

## 🔐 Security Features

- **Passwords:** SHA1 hashed (not plaintext)
- **Messages:** DES encrypted in transit
- **Network:** Socket-based with authentication
- **Kernel:** Device driver for crypto operations

## 📊 Default Configuration

| Setting | Value |
|---------|-------|
| Service Port | 5555 |
| Max Clients | 100 |
| Encryption | DES (8-byte blocks) |
| Hashing | SHA1 (20-byte digest) |
| Server | localhost (change in code) |

## 🚫 Known Limitations

- CentOS 6 32-bit only (older kernel API)
- DES is legacy (use AES in production)
- In-memory user database (not persistent)
- Max 100 concurrent users
- No group messaging
- No message history

## 🆘 Get Help

1. Check log files:
   ```bash
   tail -f /var/log/chat_daemon.log
   dmesg | grep chat_crypto
   ```

2. Verify kernel headers match kernel:
   ```bash
   uname -r
   ```

3. Recompile everything:
   ```bash
   cd ChatDriver && ./build.sh clean && ./build.sh all
   ```

4. See **INSTALL.md** for detailed troubleshooting

## 💡 Tips

- Run daemon with `nohup` to prevent termination:
  ```bash
  sudo nohup /usr/local/bin/chat_daemon > /var/log/chat_daemon.log 2>&1 &
  ```

- Monitor in real-time:
  ```bash
  tail -f /var/log/chat_daemon.log
  ```

- Stop daemon gracefully:
  ```bash
  sudo killall chat_daemon
  ```

- Remove module when done:
  ```bash
  sudo rmmod chat_crypto
  ```

---

**That's it!** You now have a working secure P2P chat application for CentOS 6 32-bit.

For issues or questions, see INSTALL.md or README.md.

# Chat Driver - P2P Chat Application for CentOS 6 32-bit

A secure, kernel-based chat application with DES encryption and SHA1 hashing, specifically designed for CentOS 6 32-bit systems.

## 📋 Features

✅ **Kernel-Level Encryption**
- DES encryption for message confidentiality
- SHA1 hashing for password security
- Custom kernel module driver (`/dev/chat_crypto`)

✅ **P2P (One-to-One) Messaging**
- Direct user-to-user communication
- Message acknowledgment system
- User authentication

✅ **Modern GUI**
- GTK2-based desktop application
- Compatible with CentOS 6
- User login/registration
- Real-time message display
- User status indicators

✅ **Production-Ready Architecture**
- Multi-threaded daemon service
- Secure socket communication
- User database with hashed passwords
- Comprehensive logging
- Error handling

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────┐
│       Chat Client (GTK GUI)                         │
│  - User Interface (Login/Register)                  │
│  - Message Send/Receive UI                         │
│  - Crypto via /dev/chat_crypto                    │
└────────────────┬────────────────────────────────────┘
                 │
        TCP/IP Socket (Port 5555)
                 │
┌────────────────▼────────────────────────────────────┐
│     Chat Daemon Service (userspace_service)        │
│  - Socket Server                                    │
│  - Client Management                               │
│  - Message Relay                                    │
│  - User Database                                    │
│  - Crypto API Calls                                │
└────────────────┬────────────────────────────────────┘
                 │
         ioctl() Calls
                 │
┌────────────────▼────────────────────────────────────┐
│   Kernel Module (/dev/chat_crypto)                 │
│  - DES Encryption/Decryption                       │
│  - SHA1 Hashing                                     │
│  - Character Device Driver                          │
└─────────────────────────────────────────────────────┘
```

## 📁 Directory Structure

```
ChatDriver/
├── kernel_module/              # Kernel driver
│   ├── chat_crypto.c          # Main kernel module (DES + SHA1)
│   └── Makefile               # Build configuration
├── userspace_service/         # Daemon service
│   └── chat_daemon.c          # Multi-threaded socket server
├── gtk_gui/                   # Desktop client
│   └── chat_client.c          # GTK2 GUI application
├── include/                   # Shared headers
│   ├── crypto_module.h        # Kernel/User-space IPC definitions
│   └── chat_protocol.h        # Network protocol definitions
├── build/                     # Build output directory
├── BUILD.md                   # Compilation instructions
├── INSTALL.md                 # Setup and deployment guide
└── README.md                  # This file
```

## 🚀 Quick Start

### 1. **Build**

```bash
cd ChatDriver/kernel_module
make

cd ../userspace_service
gcc -o chat_daemon chat_daemon.c -lpthread

cd ../gtk_gui
gcc -o chat_client chat_client.c `pkg-config --cflags --libs gtk+-2.0` -lpthread
```

### 2. **Install Kernel Module**

```bash
cd kernel_module
sudo make install

# Verify
lsmod | grep chat_crypto
ls -l /dev/chat_crypto
```

### 3. **Start Service**

```bash
sudo /path/to/chat_daemon &
```

### 4. **Run Clients**

```bash
# Terminal 1
./gtk_gui/chat_client

# Terminal 2
./gtk_gui/chat_client
```

### 5. **Test**

- Register/login with different usernames
- Send messages between clients
- Messages are encrypted with DES
- Passwords are hashed with SHA1

## 🔐 Security Features

### DES Encryption
- 8-byte key encryption
- Block cipher mode
- Automatic PKCS#7 padding
- IV support for CBC mode

### SHA1 Hashing
- Password hashing (not plaintext storage)
- Message integrity verification
- 160-bit (20-byte) hash output

### Network Security
- All messages encrypted in transit
- User authentication required
- Session tokens
- Firewall-friendly (single port 5555)

## 📊 Protocol Specification

### Packet Types

| Type | Code | Purpose |
|------|------|---------|
| PKT_REGISTER | 0x01 | User registration |
| PKT_LOGIN | 0x02 | User login |
| PKT_AUTH_RESPONSE | 0x03 | Authentication result |
| PKT_MESSAGE | 0x04 | Encrypted P2P message |
| PKT_MESSAGE_ACK | 0x05 | Message acknowledgment |
| PKT_DISCONNECT | 0x06 | Client disconnect |
| PKT_HEARTBEAT | 0x09 | Keep-alive ping |
| PKT_ERROR | 0xFF | Error indication |

### Message Format

```
struct chat_packet {
    uint32_t sender_id;           // Sender user ID
    uint32_t receiver_id;         // Receiver user ID
    uint64_t timestamp;           // Message timestamp
    uint8_t iv[8];               // DES IV
    unsigned char encrypted_msg[]; // DES-encrypted message
}
```

## 🛠️ Development

### Adding New Features

1. **New Encryption Algorithm:**
   - Add to `crypto_module.h`
   - Implement in `kernel_module/chat_crypto.c`
   - Call via new ioctl code

2. **New Message Types:**
   - Define in `chat_protocol.h`
   - Handle in daemon's `client_thread_func()`
   - Update GUI to display new msg types

3. **User Features:**
   - Database is in `userspace_service/chat_daemon.c`
   - Can replace with SQL database (SQLite for CentOS 6)

### Debugging

Enable debug output:

```bash
# Kernel module debug
# In chat_crypto.c, add: printk(KERN_DEBUG "...")

# Daemon debug
# Or run with: strace -o daemon.trace chat_daemon

# Client debug
# GTK_DEBUG=1 chat_client
```

## 📋 System Requirements

- **OS:** CentOS 6 32-bit (or compatible)
- **Kernel:** 2.6.32+
- **RAM:** 256MB+ (512MB recommended)
- **Disk:** 100MB+ free space

### Installation Requirements

- GCC compiler (gcc, gcc-c++)
- Kernel development headers (`kernel-devel`)
- GTK+ 2.0 (`gtk2-devel`)
- GLib 2.0 development libraries
- POSIX-compatible libc (glibc)

See `BUILD.md` for detailed dependency installation.

## 📝 Limitations

1. **Educational Implementation**
   - DES is simplified for learning
   - SHA1 is legacy (OK for demo, use SHA256+ in production)
   - Not FIPS 140-2 certified

2. **CentOS 6 Constraints**
   - Requires GTK2 (GTK3 not available)
   - Older kernel API
   - Limited modern library support

3. **Performance**
   - Max 100 concurrent clients (configurable)
   - Single-threaded message relay
   - In-memory user database (no persistence)

4. **Scalability**
   - Not designed for 1000+ concurrent users
   - Message history not supported
   - No load balancing

## 🔄 Future Enhancements

- [ ] Replace DES with AES-256 encryption
- [ ] Add SQLite database for user persistence
- [ ] Web-based admin dashboard
- [ ] Group chat support
- [ ] File transfer capability
- [ ] Message history/search
- [ ] Voice/video call support
- [ ] Cross-platform extensions

## 📖 Documentation

- **BUILD.md** - Compilation and build instructions
- **INSTALL.md** - Deployment and operation guide
- **chat_protocol.h** - Network protocol reference
- **crypto_module.h** - Crypto API documentation

## 🧪 Testing Checklist

- [ ] Kernel module loads without errors
- [ ] `/dev/chat_crypto` device created
- [ ] Daemon starts and listens on port 5555
- [ ] Client connects and authenticates
- [ ] Register new user works
- [ ] Login with existing user works
- [ ] Send message between two clients
- [ ] Message arrives encrypted
- [ ] Decrypt on receiver side works
- [ ] Client disconnect graceful

## 🐛 Troubleshooting

### Common Issues

**"Failed to open /dev/chat_crypto"**
```bash
# Check if kernel module is loaded
lsmod | grep chat_crypto

# Load if missing
cd kernel_module && sudo make install
```

**"Permission denied" on device**
```bash
# Fix permissions
sudo chmod 666 /dev/chat_crypto
```

**Daemon won't start**
```bash
# Run manually and check errors
/usr/local/bin/chat_daemon

# Check if port 5555 is in use
netstat -tlnp | grep 5555
```

**Client can't connect**
```bash
# Verify daemon is running
ps aux | grep chat_daemon

# Check firewall
sudo iptables -L -n | grep 5555

# Add firewall rule if needed
sudo iptables -A INPUT -p tcp --dport 5555 -j ACCEPT
```

See `INSTALL.md` for more detailed troubleshooting.

## 📄 License

Educational/Demo Project - Use at your own risk
Not recommended for production use without security audit

## 👥 Authors

Chat Driver Development Team

## 📞 Support

For issues, questions, or feature requests:
1. Check BUILD.md and INSTALL.md
2. Review error logs: `/var/log/chat_daemon.log`
3. Run diagnostic commands in INSTALL.md

---

**Last Updated:** March 2026
**Version:** 1.0
**Target OS:** CentOS 6 32-bit

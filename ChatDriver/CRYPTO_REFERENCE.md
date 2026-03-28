# Technical Reference - Chat Driver Crypto Operations

This document provides detailed technical information about the cryptographic implementations in the Chat Driver project.

## DES (Data Encryption Standard) Implementation

### Algorithm Overview

DES is a 64-bit block cipher with a 56-bit effective key (8 bytes with parity bits).

**Simplified Implementation (Educational):**
```c
// Encryption of single 8-byte block
des_encrypt_block(key[8], input[8]) → output[8]

// Internal process:
1. XOR with key[0:8]
2. Left rotate as block
3. XOR again with key (shifted)
4. Final XOR mix
```

**WARNING:** This is NOT standard DES-ECB. It's a simplified algorithm for educational purposes only. Do NOT use for production encryption.

### Usage in Chat Driver

**For Message Encryption:**

```c
// Client side - encrypting outgoing message
unsigned char plain_msg[256];       // "Hello Bob!"
unsigned char key[8];                // Symmetric key (hard-coded)
unsigned char cipher[264];           // Encrypted output (with padding)
unsigned long cipher_len;

des_encrypt(key, plain_msg, sizeof(plain_msg), cipher, &cipher_len);
// Send 'cipher' over network to server → receiver
```

**For Message Decryption:**

```c
// Server/Receiver side - decrypting received message
unsigned char cipher[264];           // Received encrypted data
unsigned char key[8];                // Same key
unsigned char plain_msg[256];        // Decrypted output
unsigned long plain_len;

des_decrypt(key, cipher, cipher_len, plain_msg, &plain_len);
// Display plain_msg in chat window
```

### Padding Scheme

**PKCS#7 Padding:**
```
Message:  "Hello" (5 bytes)
Padded:   "Hello" + 0x03 + 0x03 + 0x03  (8 bytes total)
          (pad length = 3, so add 3 bytes of value 0x03)

When decrypting:
1. Check last byte (0x03)
2. Remove last 3 bytes
3. Result: "Hello"
```

### Key Management

**Current Implementation:**
```c
// Hard-coded symmetric key (NOT SECURE)
unsigned char DES_KEY[] = {
    0x01, 0x23, 0x45, 0x67,
    0x89, 0xAB, 0xCD, 0xEF
};
```

**Issues:**
- Same key for all users
- No key exchange mechanism
- Visible in source code
- Not random

**Production Improvements:**
- Per-user session key
- Diffie-Hellman key exchange
- Random key generation
- Secure key storage

## SHA1 Hash Function

### Algorithm Overview

SHA1 produces a 160-bit (20-byte) hash output from arbitrary input.

**Process:**
```
Input: User password (string)
  ↓
Initialize: h0-h4 (five 32-bit constants)
  ↓
Message Schedule: Expand message into 80 32-bit words
  ↓
80 Rounds: Mix h0-h4 with message words
  ↓
Final: Output h0-h4 as 160-bit hash
```

### Simplified Implementation

The kernel module implements a simplified SHA1:

```c
/**
 * SHA1 Context Structure
 */
typedef struct {
    uint32_t h[5];      // State variables (20 bytes)
    uint32_t w[80];     // Message schedule (320 bytes)
    uint64_t size;      // Total bits processed
} sha1_ctx_t;

/**
 * Three-step process:
 * 1. sha1_init() - Initialize h[] constants
 * 2. sha1_update() - Process input data
 * 3. sha1_final() - Output final hash
 */
```

### Usage in Chat Driver

**Password Hashing (Registration):**

```
User Registration:
  alice@localhost → password: "password123"
                     ↓
              sha1_hash("password123")
                     ↓
         20-byte digest: 482C811DA5D5B4BC6D497FFA98491E38
                     ↓
         Store in user database
```

**Password Verification (Login):**

```
User Login:
  alice@localhost → password_input: "password123"
                     ↓
              sha1_hash("password123")
                     ↓
         20-byte digest: 482C811DA5D5B4BC6D497FFA98491E38
                     ↓
         Compare with stored hash (from registration)
                     ↓
              Match? → Login success
```

### Hash Properties

**Output Size:** 160 bits (20 bytes)
```
hex: 40 characters
binary: 160 bits
```

**Deterministic:**
```
sha1("hello") = 0xAAF4C61DDCC5E8A2DABEDE0F3B482CD9... (always same)
```

**Avalanche Effect (good property):**
```
sha1("hello")  = 0xAAF4C61DDCC5E8A2...
sha1("hallo")  = 0xEC2BA8557C4178... (completely different)
sha1("hello ") = 0xC3499C2729730A... (completely different)
```

## Kernel Module Communication

### ioctl Interface

The kernel module exposes crypto operations via ioctl calls.

**Function:** `dev_ioctl(file, cmd, arg)`

**Supported Commands:**

```c
// DES Encryption
CRYPTO_IOCTL_DES_ENCRYPT
  Input:  des_request { key, input_data, input_len }
  Output: des_request { output_data, output_len }

// DES Decryption
CRYPTO_IOCTL_DES_DECRYPT
  Input:  des_request { key, input_data, input_len }
  Output: des_request { output_data, output_len }

// SHA1 Hashing
CRYPTO_IOCTL_SHA1_HASH
  Input:  sha1_request { input_data, input_len }
  Output: sha1_request { digest[20] }
```

### Data Structures

**DES Request:**
```c
struct des_request {
    unsigned char key[8];              // 8-byte DES key
    unsigned char input[4096];         // Input plaintext/ciphertext
    unsigned char output[4096];        // Output after crypto op
    unsigned long input_len;           // Input length (bytes)
    unsigned long output_len;          // Output length (bytes)
    int mode;                         // 0=encrypt, 1=decrypt
};
```

**SHA1 Request:**
```c
struct sha1_request {
    unsigned char input[4096];         // Input to hash
    unsigned char digest[20];          // Output: SHA1 hash
    unsigned long input_len;           // Input length
};
```

### Usage Example (C Code)

```c
// Encrypt a message using kernel module
#include <fcntl.h>
#include <sys/ioctl.h>
#include "crypto_module.h"

int crypto_fd = open("/dev/chat_crypto", O_RDWR);

unsigned char key[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
unsigned char plaintext[16] = "Hello World!!!!!";
struct des_request req;

memcpy(req.key, key, 8);
memcpy(req.input, plaintext, 16);
req.input_len = 16;
req.mode = 0;  // Encrypt

// Call kernel module
ioctl(crypto_fd, CRYPTO_IOCTL_DES_ENCRYPT, &req);

// Result in req.output (16 bytes encrypted)
printf("Ciphertext: ");
for (int i = 0; i < 16; i++) {
    printf("%02X", req.output[i]);
}
```

## Performance Considerations

### Encryption Speed

**Approximate Throughput:**
- Per-block: ~1 microsecond (kernel context)
- Network latency: ~1-10 milliseconds (dominates)
- Overall: Limited by network I/O, not crypto

### Memory Usage

**Per-Connection:**
```
Kernel module: ~50KB constant
Daemon state: 
  - Per client: ~500 bytes
  - 100 clients: ~50KB
  - Database: ~5KB per user

Client application:
  - GTK library: ~10-20MB
  - Application: ~5MB
  - Per window: negligible
```

### CPU Usage

**Daemon Processing:**
```
Idle: <1%
Relaying messages: 1-5% (per message)
Crypto operations: Kernel-space (minimal user-space CPU)
```

## Security Analysis

### Strengths

✅ Uses hash functions for password storage
✅ Separates auth from message crypto
✅ Kernel-space crypto operations
✅ Per-message encryption

### Weaknesses

❌ DES is legacy (56-bit effective key)
❌ Same key for all users
❌ No forward secrecy
❌ No TLS/SSL transport layer
❌ Hard-coded keys
❌ SHA1 is cryptographically weak
❌ No mutual authentication
❌ No message integrity check
❌ No replay attack protection

### Attack Scenarios

**1. Man-in-the-Middle (Network Level)**
- Attacker intercepts unencrypted socket
- Can read metadata but not message content (DES encrypted)
- **Mitigation:** Use TLS for transport layer

**2. Dictionary Attack on Passwords**
- Attacker knows SHA1 algorithms
- Can pre-compute hashes of common passwords
- **Mitigation:** Use salt + bcrypt/PBKDF2

**3. Brute Force DES Key**
- 2^56 possible keys (~72 quadrillion)
- Modern hardware can try billions/second
- Practically breakable in hours
- **Mitigation:** Use AES-256

**4. Replay Attack**
- Attacker captures encrypted message
- Replays it multiple times
- Server would relay same message
- **Mitigation:** Add sequence numbers + timestamp validation

## Cryptographic Standards Violations

### DES Implementation
```
Standard:     DES-ECB (wrong for security)
Should use:   DES-CBC with IV (but still obsolete)
Better:       AES-128-CBC or AES-256-GCM
```

### SHA1 for Password Hashing
```
Current:      SHA1(password)
Problem:      No salt, fast (billions/sec attempts)
Better:       bcrypt($2b$12$..., 12)
OR:           PBKDF2(SHA256, iterations=100000)
OR:           Argon2 (modern, memory-hard)
```

### Session Key Management
```
Current:      Hard-coded same key for all
Should use:   Dynamic per-session key exchange
Method:       Diffie-Hellman or ECDH
Example:      TLS 1.3 style ephemeral keys
```

## Testing Crypto Operations

### Test Vector for DES

(Using simplified implementation - NOT standard DES)

```
Key:   01 23 45 67 89 AB CD EF
Input: 00 11 22 33 44 55 66 77
Output: [encrypted bytes - implementation-specific]
```

Note: This will NOT match standard DES test vectors due to simplified algorithm.

### Test Vector for SHA1

(Standard SHA-1, if implemented correctly)

```
Input:  "abc"
Output: A9993E364706816ABA3E25717850C26C9CD0D89D

Input:  ""  (empty)
Output: DA39A3EE5E6B4B0D3255BFEF95601890AFD80709

Input:  "The quick brown fox jumps over the lazy dog"
Output: 2FD4E1C67A2D28FED849EE1BB76E7BE9
         DFC6D4D87D77966AD8F05E91B21B21A17
```

## Debugging Crypto Operations

### Enable Kernel Debug Logging

In `kernel_module/chat_crypto.c`, add:

```c
printk(KERN_DEBUG "DES encrypt: input_len=%lu\n", input_len);
printk(KERN_DEBUG "SHA1: processing %lu bytes\n", len);
```

Then view logs:
```bash
sudo dmesg | grep -i des
sudo dmesg | grep -i sha1
tail -f /var/log/messages
```

### Test Crypto from User-Space

```c
// Create test program
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "crypto_module.h"

int main() {
    int fd = open("/dev/chat_crypto", O_RDWR);
    
    // Test SHA1
    sha1_request req;
    strcpy(req.input, "test");
    req.input_len = 4;
    
    ioctl(fd, CRYPTO_IOCTL_SHA1_HASH, &req);
    
    printf("SHA1 of 'test': ");
    for (int i = 0; i < 20; i++) {
        printf("%02X", req.digest[i]);
    }
    printf("\n");
    
    close(fd);
}
```

Compile and run:
```bash
gcc -o crypto_test crypto_test.c
./crypto_test
```

## Future Cryptographic Improvements

### Phase 1: Quick Wins
- [ ] Replace SHA1 with SHA256
- [ ] Add HMAC-SHA256 for message auth
- [ ] Random salt for passwords
- [ ] Use bcrypt library

### Phase 2: Modern Crypto
- [ ] Replace DES with AES-256-GCM
- [ ] Add Diffie-Hellman key exchange
- [ ] Session-based keys
- [ ] Message sequence numbers

### Phase 3: Production-Grade
- [ ] TLS 1.3 for transport
- [ ] Perfect Forward Secrecy (PFS)
- [ ] ECDSA for authentication
- [ ] X.509 certificates
- [ ] Certificate pinning

---

**Document Version:** 1.0
**Applies To:** Chat Driver v1.0
**Last Updated:** March 2026

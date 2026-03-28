# Build Instructions for CentOS 6 32-bit

This document provides detailed compilation and installation instructions for the Chat Driver project on CentOS 6 32-bit.

## Prerequisites

### System Requirements
- CentOS 6 32-bit (2.6.32 kernel or compatible)
- 512MB+ RAM
- GCC compiler (gcc, gcc-c++)
- Development tools
- GTK+ 2.0 development libraries

### Install Build Dependencies

```bash
# Update system
sudo yum update -y

# Install development tools
sudo yum groupinstall -y "Development Tools"

# Install kernel development headers (required for kernel module)
sudo yum install -y kernel-devel-$(uname -r)
sudo yum install -y kernel-headers-$(uname -r)

# Install GTK+ 2.0 development library
sudo yum install -y gtk2-devel

# Install other required libraries
sudo yum install -y glib2-devel
sudo yum install -y pango-devel
sudo yum install -y atk-devel
sudo yum install -y cairo-devel
```

## Build Steps

### 1. Build Kernel Module

```bash
cd ChatDriver/kernel_module
make clean
make

# You should see output like:
# Building modules...
# Leaving directory ...
# cp chat_crypto.ko ../build/
```

If compilation fails with undefined references, verify kernel headers match your running kernel:
```bash
uname -r
```

### 2. Build User-space Service

```bash
cd ../userspace_service
gcc -o chat_daemon chat_daemon.c -lpthread -Wall

# Create log directory
sudo mkdir -p /var/log/
```

### 3. Build GTK Client

```bash
cd ../gtk_gui

# Check GTK2 is installed
pkg-config --cflags --libs gtk+-2.0

# Compile
gcc -o chat_client chat_client.c `pkg-config --cflags --libs gtk+-2.0` -lpthread -Wall
```

### 4. Copy Build Artifacts

```bash
mkdir -p ../build
cp chat_daemon.c ../build/
cp gtk_gui/chat_client ../build/
cp kernel_module/chat_crypto.ko ../build/
```

## Verify Build

```bash
ls -la build/
# Should contain:
# - chat_crypto.ko (kernel module)
# - chat_daemon (userspace daemon)
# - chat_client (GTK GUI client)
```

## Troubleshooting

### Kernel Header Mismatch
If you get "unresolved symbol" errors:
```bash
# Verify headers match your kernel
uname -r
# Then reinstall exact version
sudo yum install -y kernel-devel-EXACT_VERSION
```

### GTK Not Found
```bash
# Verify GTK2 is installed
pkg-config --exists gtk+-2.0 && echo "GTK2 found"

# If not found, install:
sudo yum install -y gtk2-devel
```

### Missing libpthread
```bash
# Already part of glibc, try explicit linking:
gcc ... -lpthread
```

## Next Steps

See INSTALL.md for installation and running the application.

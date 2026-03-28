#!/bin/bash
# Master build script for Chat Driver project
# Target: CentOS 6 32-bit
# Usage: ./build.sh [all|kernel|daemon|client|clean|install]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
KERNEL_DIR="$SCRIPT_DIR/kernel_module"
DAEMON_DIR="$SCRIPT_DIR/userspace_service"
CLIENT_DIR="$SCRIPT_DIR/gtk_gui"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
print_status() {
    echo -e "${GREEN}[*]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for GCC
    if ! command -v gcc &> /dev/null; then
        print_error "GCC not found. Install with: sudo yum install -y gcc gcc-c++"
        exit 1
    fi
    
    # Check for kernel headers
    if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
        print_error "Kernel headers not found for $(uname -r)"
        echo "Install with: sudo yum install -y kernel-devel-$(uname -r)"
        exit 1
    fi
    
    # Check for GTK2
    if ! pkg-config --exists gtk+-2.0 2>/dev/null; then
        print_warning "GTK2 not found. Some features may not compile."
        print_warning "Install with: sudo yum install -y gtk2-devel"
    fi
    
    print_status "Dependencies OK"
}

build_kernel_module() {
    print_status "Building kernel module..."
    
    if [ ! -f "$KERNEL_DIR/Makefile" ]; then
        print_error "Kernel module Makefile not found"
        return 1
    fi
    
    cd "$KERNEL_DIR"
    make clean
    make
    
    print_status "Kernel module built successfully"
    print_status "Output: $KERNEL_DIR/chat_crypto.ko"
}

build_daemon() {
    print_status "Building userspace daemon..."
    
    if [ ! -f "$DAEMON_DIR/chat_daemon.c" ]; then
        print_error "Daemon source not found"
        return 1
    fi
    
    cd "$DAEMON_DIR"
    gcc -o chat_daemon chat_daemon.c -lpthread -Wall
    
    print_status "Daemon built successfully"
    print_status "Output: $DAEMON_DIR/chat_daemon"
}

build_client() {
    print_status "Building GTK client..."
    
    if [ ! -f "$CLIENT_DIR/chat_client.c" ]; then
        print_error "Client source not found"
        return 1
    fi
    
    # Check if pkg-config can find gtk+-2.0
    if ! pkg-config --exists gtk+-2.0 2>/dev/null; then
        print_error "GTK2 not found. Install with: sudo yum install -y gtk2-devel"
        return 1
    fi
    
    cd "$CLIENT_DIR"
    gcc -o chat_client chat_client.c $(pkg-config --cflags --libs gtk+-2.0) -lpthread -Wall
    
    print_status "Client built successfully"
    print_status "Output: $CLIENT_DIR/chat_client"
}

copy_artifacts() {
    print_status "Copying artifacts to $BUILD_DIR..."
    
    mkdir -p "$BUILD_DIR"
    
    if [ -f "$KERNEL_DIR/chat_crypto.ko" ]; then
        cp "$KERNEL_DIR/chat_crypto.ko" "$BUILD_DIR/"
        print_status "Copied kernel module"
    fi
    
    if [ -f "$DAEMON_DIR/chat_daemon" ]; then
        cp "$DAEMON_DIR/chat_daemon" "$BUILD_DIR/"
        print_status "Copied daemon"
    fi
    
    if [ -f "$CLIENT_DIR/chat_client" ]; then
        cp "$CLIENT_DIR/chat_client" "$BUILD_DIR/"
        print_status "Copied client"
    fi
    
    print_status "All artifacts in $BUILD_DIR"
}

install_kernel_module() {
    print_status "Installing kernel module..."
    
    if [ ! -f "$KERNEL_DIR/chat_crypto.ko" ]; then
        print_error "Kernel module not built. Run: $0 kernel"
        return 1
    fi
    
    cd "$KERNEL_DIR"
    sudo make install
    
    print_status "Module installed"
    sleep 1
    
    if lsmod | grep -q chat_crypto; then
        print_status "Module verified loaded"
    else
        print_warning "Module may not have loaded automatically"
        echo "Try: sudo insmod $KERNEL_DIR/chat_crypto.ko"
    fi
}

install_daemon() {
    print_status "Installing daemon..."
    
    if [ ! -f "$DAEMON_DIR/chat_daemon" ]; then
        print_error "Daemon not built. Run: $0 daemon"
        return 1
    fi
    
    sudo cp "$DAEMON_DIR/chat_daemon" /usr/local/bin/
    print_status "Daemon installed to /usr/local/bin/chat_daemon"
}

install_client() {
    print_status "Installing client..."
    
    if [ ! -f "$CLIENT_DIR/chat_client" ]; then
        print_error "Client not built. Run: $0 client"
        return 1
    fi
    
    sudo cp "$CLIENT_DIR/chat_client" /usr/local/bin/
    print_status "Client installed to /usr/local/bin/chat_client"
}

clean_all() {
    print_status "Cleaning build artifacts..."
    
    cd "$KERNEL_DIR"
    make clean
    
    if [ -f "$DAEMON_DIR/chat_daemon" ]; then
        rm "$DAEMON_DIR/chat_daemon"
    fi
    
    if [ -f "$CLIENT_DIR/chat_client" ]; then
        rm "$CLIENT_DIR/chat_client"
    fi
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    print_status "Clean complete"
}

show_usage() {
    cat << EOF
Chat Driver - Build Script for CentOS 6 32-bit

Usage: $0 [command]

Commands:
  all              Build kernel, daemon, and client (default)
  kernel           Build kernel module only
  daemon           Build userspace daemon only
  client           Build GTK client only
  clean            Remove all build artifacts
  install-module   Install kernel module (sudo required)
  install-daemon   Copy daemon to /usr/local/bin
  install-client   Copy client to /usr/local/bin
  install-all      Install all components
  help             Show this help message

Examples:
  $0                    # Build everything
  $0 kernel             # Build kernel module only
  $0 install-all        # Build and install everything
  $0 clean              # Remove build artifacts

EOF
}

# Main
case "${1:-all}" in
    all)
        check_dependencies
        build_kernel_module
        build_daemon
        build_client
        copy_artifacts
        ;;
    kernel)
        check_dependencies
        build_kernel_module
        ;;
    daemon)
        build_daemon
        ;;
    client)
        check_dependencies
        build_client
        ;;
    clean)
        clean_all
        ;;
    install-module)
        install_kernel_module
        ;;
    install-daemon)
        install_daemon
        ;;
    install-client)
        install_client
        ;;
    install-all)
        check_dependencies
        build_kernel_module
        build_daemon
        build_client
        install_kernel_module
        install_daemon
        install_client
        print_status "All components installed successfully"
        ;;
    help)
        show_usage
        ;;
    *)
        print_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac

exit 0

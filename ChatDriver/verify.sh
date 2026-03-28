#!/bin/bash
# Verification Script - Chat Driver Installation Checker
# Run this to verify your Chat Driver installation is correct

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

CHECKS_PASSED=0
CHECKS_FAILED=0

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

check_pass() {
    echo -e "${GREEN}[✓]${NC} $1"
    ((CHECKS_PASSED++))
}

check_fail() {
    echo -e "${RED}[✗]${NC} $1"
    ((CHECKS_FAILED++))
}

check_warn() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Main checks
print_header "Chat Driver Installation Verification"

echo ""
echo "Checking system requirements..."

# Check OS
if [ -f /etc/redhat-release ]; then
    OS_INFO=$(cat /etc/redhat-release)
    check_pass "CentOS/RHEL detected: $OS_INFO"
else
    check_fail "Not CentOS/RHEL based system"
fi

# Check kernel
KERNEL=$(uname -r)
if [[ $KERNEL == 2.6.32* ]]; then
    check_pass "Kernel version: $KERNEL"
else
    check_warn "Kernel version: $KERNEL (expected 2.6.32.x for CentOS 6)"
fi

# Check architecture
ARCH=$(uname -m)
if [ "$ARCH" = "i686" ]; then
    check_pass "32-bit architecture detected"
elif [ "$ARCH" = "i386" ]; then
    check_pass "32-bit architecture detected"
else
    check_warn "Architecture: $ARCH (expected i686 for 32-bit)"
fi

echo ""
echo "Checking dependencies..."

# Check GCC
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc --version | head -1)
    check_pass "GCC found: $GCC_VERSION"
else
    check_fail "GCC not found"
fi

# Check kernel headers
if [ -d "/lib/modules/$(uname -r)/build" ]; then
    check_pass "Kernel headers found for $(uname -r)"
else
    check_fail "Kernel headers NOT found for $(uname -r)"
fi

# Check GTK2
if pkg-config --exists gtk+-2.0 2>/dev/null; then
    GTK_VERSION=$(pkg-config --modversion gtk+-2.0)
    check_pass "GTK2 found: version $GTK_VERSION"
else
    check_fail "GTK2 development libraries not installed"
fi

# Check pthread library
if ldconfig -p | grep -q libpthread.so; then
    check_pass "POSIX threads (pthreads) available"
else
    check_fail "POSIX threads not available"
fi

echo ""
echo "Checking Chat Driver installation..."

# Check kernel module
if lsmod | grep -q chat_crypto; then
    check_pass "Kernel module loaded (chat_crypto)"
else
    check_warn "Kernel module not loaded (use: sudo make -C kernel_module install)"
fi

# Check device
if [ -c /dev/chat_crypto ]; then
    check_pass "Crypto device created (/dev/chat_crypto)"
    
    # Check permissions
    if [ -r /dev/chat_crypto ] && [ -w /dev/chat_crypto ]; then
        check_pass "Crypto device readable/writable"
    else
        check_fail "Crypto device has insufficient permissions"
    fi
else
    check_fail "Crypto device NOT found (/dev/chat_crypto)"
fi

echo ""
echo "Checking daemon service..."

# Check if daemon binary exists
if [ -f /usr/local/bin/chat_daemon ]; then
    check_pass "Daemon binary found (/usr/local/bin/chat_daemon)"
else
    check_warn "Daemon binary not installed (expected: /usr/local/bin/chat_daemon)"
fi

# Check if daemon is running
if pgrep -f chat_daemon > /dev/null; then
    chat_daemon_pid=$(pgrep -f chat_daemon)
    check_pass "Daemon process running (PID: $chat_daemon_pid)"
    
    # Check if listening on port 5555
    if netstat -tlnp 2>/dev/null | grep -q 5555; then
        check_pass "Daemon listening on port 5555"
    else
        check_warn "Daemon not listening on port 5555"
    fi
else
    check_warn "Daemon process not running (start with: sudo /usr/local/bin/chat_daemon)"
fi

# Check log file
if [ -f /var/log/chat_daemon.log ]; then
    check_pass "Daemon log file exists (/var/log/chat_daemon.log)"
    log_lines=$(wc -l < /var/log/chat_daemon.log)
    check_pass "Log has $log_lines entries"
else
    check_warn "Daemon log file not found"
fi

echo ""
echo "Checking client application..."

# Check if client binary exists
if [ -f /usr/local/bin/chat_client ]; then
    check_pass "Client binary found (/usr/local/bin/chat_client)"
else
    check_warn "Client binary not installed (expected: /usr/local/bin/chat_client)"
fi

echo ""
echo "Checking source code..."

# Check if source structure exists
if [ -f "kernel_module/chat_crypto.c" ]; then
    check_pass "Kernel module source found"
else
    check_warn "Kernel module source not found in current directory"
fi

if [ -f "userspace_service/chat_daemon.c" ]; then
    check_pass "Daemon source found"
else
    check_warn "Daemon source not found in current directory"
fi

if [ -f "gtk_gui/chat_client.c" ]; then
    check_pass "Client source found"
else
    check_warn "Client source not found in current directory"
fi

# Check documentation
if [ -f "README.md" ] || [ -f "QUICKSTART.md" ]; then
    check_pass "Documentation found"
else
    check_warn "Documentation not found"
fi

echo ""
echo "Checking network configuration..."

# Check if port 5555 can be used
if netstat -tlnp 2>/dev/null | grep -q LISTEN; then
    listening_ports=$(netstat -tln | grep LISTEN | wc -l)
    check_pass "Network interface active ($listening_ports listening ports)"
else
    check_warn "No listening ports detected"
fi

# Check iptables
if [ -x "$(command -v iptables)" ]; then
    if sudo iptables -L -n 2>/dev/null | grep -q 5555; then
        check_pass "Port 5555 allowed in firewall"
    else
        check_warn "Port 5555 may be blocked by firewall (check: sudo iptables -L)"
    fi
fi

echo ""
print_header "Summary"
echo -e "Tests passed: ${GREEN}$CHECKS_PASSED${NC}"
echo -e "Tests failed: ${RED}$CHECKS_FAILED${NC}"

echo ""
if [ $CHECKS_FAILED -eq 0 ]; then
    if [ $CHECKS_PASSED -ge 8 ]; then
        echo -e "${GREEN}✓ Installation looks good!${NC}"
        echo ""
        echo "Next steps:"
        echo "1. Run daemon: sudo /usr/local/bin/chat_daemon"
        echo "2. Run client: /usr/local/bin/chat_client"
        echo "3. Register/login and test messaging"
        exit 0
    fi
fi

if [ $CHECKS_FAILED -gt 0 ]; then
    echo -e "${RED}✗ Some checks failed. See details above.${NC}"
    echo ""
    echo "To fix:"
    echo "1. Check BUILD.md for compilation instructions"
    echo "2. Check INSTALL.md for installation steps"
    echo "3. Run troubleshooting commands in INSTALL.md"
    exit 1
fi

if [ $CHECKS_PASSED -lt 8 ]; then
    echo -e "${YELLOW}! Installation may be incomplete.${NC}"
    echo ""
    echo "Please check:"
    echo "1. Are dependencies installed? (gcc, kernel-devel, gtk2-devel)"
    echo "2. Was the kernel module compiled and installed?"
    echo "3. Was the daemon and client built?"
    echo "4. Is the daemon running?"
    exit 1
fi

exit 0

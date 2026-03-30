# Makefile for ChatDriver Kernel Crypto Module
# Builds kernel module using kernel crypto API (SHA1 and DES)
# Optimized for CentOS 6 (kernel 2.6.32+)

# Kernel version and build directory
KVERSION := $(shell uname -r)
KBUILD_DIR := /lib/modules/$(KVERSION)/build

# Source files - kernel crypto API (no custom implementations)
obj-m := chat_crypto.o

# Module name
MODULE_NAME := chat_crypto
KO_FILE := $(MODULE_NAME).ko

# Build flags for CentOS 6
EXTRA_CFLAGS := -I$(PWD)/../include
MAKEFLAGS := -w

# Default target
all: module test

# Build the kernel module
module:
	@echo "Building kernel module for $(KVERSION)..."
	@echo "Using kernel crypto API (SHA1, DES)"
	@if [ ! -d "$(KBUILD_DIR)" ]; then \
		echo "Error: Kernel source not found at $(KBUILD_DIR)"; \
		echo "Install kernel-devel-$(KVERSION)"; \
		exit 1; \
	fi
	@echo "Checking kernel build environment..."
	make -C $(KBUILD_DIR) M=$(PWD) modules 2>&1 | grep -v "missing-syscalls" || true
	@if [ ! -f "$(KO_FILE)" ]; then \
		echo "[!] Building with alternative method for CentOS 6..."; \
		cd $(KBUILD_DIR) && make modules M=$(PWD) 2>&1 | grep -v "missing-syscalls"; \
	fi
	@if [ -f "$(KO_FILE)" ]; then \
		echo "[✓] Built kernel module: $(KO_FILE)"; \
		ls -la $(KO_FILE); \
	else \
		echo "[!] Build may have failed - checking artifacts..."; \
		ls -la *.ko 2>/dev/null || echo "No .ko files found"; \
	fi

# Install the kernel module
install: module
	@echo "Installing kernel module..."
	mkdir -p /lib/modules/$(KVERSION)/kernel/drivers/chat/
	cp $(KO_FILE) /lib/modules/$(KVERSION)/kernel/drivers/chat/
	depmod -a
	modprobe $(MODULE_NAME)
	@echo "[✓] Kernel module installed and loaded"
	@echo "Device available at: /dev/kma_crypto"
	ls -la /dev/kma_crypto 2>/dev/null || echo "Note: Device node auto-created by miscdevice"

# Uninstall the kernel module
uninstall:
	@echo "Uninstalling kernel module..."
	modprobe -r $(MODULE_NAME) 2>/dev/null || true
	rm -f /lib/modules/$(KVERSION)/kernel/drivers/chat/$(KO_FILE)
	depmod -a
	@echo "[✓] Kernel module uninstalled"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	make -C $(KBUILD_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod.o Module.symvers modules.order
	rm -f chat_crypto_test
	@echo "[✓] Clean complete"

# Build test application
test: chat_crypto_test
	@echo "[✓] Test app built: ./chat_crypto_test"

chat_crypto_test: chat_crypto_test.c
	@echo "Building test application..."
	gcc -o chat_crypto_test chat_crypto_test.c
	chmod +x chat_crypto_test
	@echo "[✓] Test app built successfully"

# Distclean: clean + uninstall
distclean: uninstall clean
	@echo "[✓] Full cleanup complete"

# Help target
help:
	@echo "ChatDriver Kernel Crypto Module (Kernel Crypto API)"
	@echo ""
	@echo "Build Targets:"
	@echo "  make              - Build kernel module and test app"
	@echo "  make module       - Build kernel module only"
	@echo "  make test         - Build test application only"
	@echo "  sudo make install - Build, install, and load module"
	@echo "  sudo make uninstall - Unload and remove module"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean     - Clean + Uninstall"
	@echo ""
	@echo "Module Components:"
	@echo "  - chat_crypto.c: Main kernel module (ioctl handler)"
	@echo "  - Uses kernel crypto API: SHA1, DES (ECB mode)"
	@echo "  - Device: /dev/kma_crypto (miscdevice)"
	@echo ""
	@echo "Test Application:"
	@echo "  ./chat_crypto_test sha1 <text>           - Hash text"
	@echo "  ./chat_crypto_test enc <text> <key>      - Encrypt (DES)"
	@echo "  ./chat_crypto_test dec <hex> <key>       - Decrypt (DES)"
	@echo ""
	@echo "Requirements:"
	@echo "  - kernel-devel package"
	@echo "  - kernel-headers package"
	@echo "  - GCC compiler"
	@echo ""
	@echo "Examples:"
	@echo "  make all"
	@echo "  sudo make install"
	@echo "  ./chat_crypto_test sha1 'hello'"
	@echo "  ./chat_crypto_test enc 'hello' 'password'"
	@echo ""

.PHONY: all module test install uninstall clean distclean help


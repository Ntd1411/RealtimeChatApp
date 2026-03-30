# Makefile for ChatDriver Kernel Crypto Module
# Builds kernel module with custom SHA1 and DES implementations
# Optimized for CentOS 6 (kernel 2.6.32)

# Kernel version and build directory
KVERSION := $(shell uname -r)
KBUILD_DIR := /lib/modules/$(KVERSION)/build

# Source files - include SHA1 and DES implementations
obj-m := chat_crypto.o
chat_crypto-objs := chat_crypto.o sha1.o des.o

# Module name
MODULE_NAME := chat_crypto
KO_FILE := $(MODULE_NAME).ko

# Build flags for CentOS 6
EXTRA_CFLAGS := -I$(PWD)/../include
MAKEFLAGS := -w

# Default target
all: module

# Build the kernel module
module:
	@echo "Building kernel module for $(KVERSION)..."
	@echo "Including: chat_crypto.c, sha1.c, des.c"
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
	@echo "Device created at: /dev/chat_crypto"
	ls -la /dev/chat_crypto 2>/dev/null || echo "Note: Device will appear after first use"

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
	@echo "[✓] Clean complete"

# Distclean: clean + uninstall
distclean: uninstall clean
	@echo "[✓] Full cleanup complete"

# Help target
help:
	@echo "ChatDriver Kernel Crypto Module (Custom SHA1 & DES)"
	@echo ""
	@echo "Build Targets:"
	@echo "  make              - Build kernel module"
	@echo "  sudo make install - Build and install (load into kernel)"
	@echo "  sudo make uninstall - Unload and remove module"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean     - Clean + Uninstall"
	@echo ""
	@echo "Module Components:"
	@echo "  - chat_crypto.c: Main module (ioctl handler)"
	@echo "  - sha1.c/h: SHA1 implementation (RFC 3174)"
	@echo "  - des.c/h: DES implementation (FIPS 46-3)"
	@echo ""
	@echo "Requirements:"
	@echo "  - kernel-devel package"
	@echo "  - kernel-headers package"
	@echo "  - GCC compiler"
	@echo ""

.PHONY: all module install uninstall clean distclean help


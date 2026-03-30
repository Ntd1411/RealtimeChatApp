CC = gcc
CXX = g++
CXXFLAGS = -fPIC -Wall -Wextra -O2 -I../include
LDFLAGS = -shared

# Library settings
LIB_NAME = libcryptodriver
LIB_TARGET = $(LIB_NAME).so
STATIC_LIB = $(LIB_NAME).a
INSTALL_PATH = /usr/local/lib
INCLUDE_INSTALL_PATH = /usr/local/include

# Source files
SOURCES = cryptodriver.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = cryptodriver.h

# Build rules
all: $(LIB_TARGET) $(STATIC_LIB)

$(LIB_TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "[✓] Built shared library: $@"

$(STATIC_LIB): $(OBJECTS)
	ar rcs $@ $^
	@echo "[✓] Built static library: $@"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "[✓] Compiled: $@"

# Installation
install: $(LIB_TARGET) $(STATIC_LIB)
	mkdir -p $(INSTALL_PATH)
	mkdir -p $(INCLUDE_INSTALL_PATH)
	cp $(LIB_TARGET) $(INSTALL_PATH)/
	cp $(STATIC_LIB) $(INSTALL_PATH)/
	cp $(HEADERS) $(INCLUDE_INSTALL_PATH)/
	ldconfig
	@echo "[✓] Installed to $(INSTALL_PATH) and $(INCLUDE_INSTALL_PATH)"

# Uninstall
uninstall:
	rm -f $(INSTALL_PATH)/$(LIB_TARGET)
	rm -f $(INSTALL_PATH)/$(STATIC_LIB)
	rm -f $(INCLUDE_INSTALL_PATH)/$(HEADERS)
	ldconfig
	@echo "[✓] Uninstalled from system"

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(LIB_TARGET) $(STATIC_LIB)
	@echo "[✓] Cleaned build artifacts"

# Full clean (includes installed files)
distclean: clean uninstall

.PHONY: all install uninstall clean distclean

### Software Requirements

```bash
sudo yum install -y epel-release
# Qt5 development files
sudo yum install -y \
    qt5-qtbase-devel \
    qt5-qtwebsockets-devel \
    qt5-qttools-devel

# Build tools and compilers
sudo yum groupinstall -y "Development Tools"

# Additional build utilities
sudo yum install -y \
    cmake3 \
    kernel-devel-$(uname -r) \
    kernel-headers-$(uname -r)
```

### Verification

```bash
# Check Qt5 installation
pkg-config --modversion Qt5Core

# Check cmake
cmake3 --version

# Check gcc/g++
gcc --version
g++ --version
```

---

## 🚀 Build & Installation

### Step 1: Build and Install Kernel Module

```bash
cd ChatDriver/kernel_module
make
sudo make install
./chat_crypto_test sha1 "hello"
```

**What gets built:**
- `chat_crypto.ko` - Main kernel module with ioctl device driver
- `chat_crypto_test` - Test

**Verify Installation:**
```bash
sudo dmesg | tail
cat /proc/devices
lsmod | grep chat_crypto
# Expected output: chat_crypto    XXXX  0

ls -la /dev/chat_crypto
# Expected: crw--w----  1 root root  XX,  XX  ...
```

### Step 2: Build Qt Application

The Qt application includes a direct kernel crypto client that calls `/dev/chat_crypto` via ioctl.

```bash
cd ChatDriver/qt_gui
mkdir build && cd build
cmake3 ..
make
```

**Expected Output:**
```
[100%] Built target chatapp
```

**Verify Executable:**
```bash
ls -la ./chatapp
file ./chatapp
```

**Build Configuration:**
```cmake
# CMakeLists.txt uses:
# - kernel_crypto_client.h/cpp (direct ioctl to kernel)
# - crypto_module.h (interface definitions)
# - No external crypto library dependency
```

### Step 3: Ensure Backend is Running

The backend must be running on port 3000 for the Qt app to connect.

```bash
# On backend server
cd RealtimeChatApp/backend
npm install          # Install dependencies
npm start            # Start Node.js server
# Expected: Server listening on http://localhost:3000
```

### Step 4: Running the Application Chat

```bash
cd RealtimeChatApp/ChatDriver/qt_gui/build
./chatapp
```

### Quick start
```bash
cd RealtimeChatApp/ChatDriver/kernel_module
make clean
make
make install
cd ~
cd RealtimeChatApp/ChatDriver/qt_gui/build
./chatapp
```
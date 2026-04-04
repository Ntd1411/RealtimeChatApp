### Software Requirements (Ubuntu 24)

```bash
# Update package manager
sudo apt update && sudo apt upgrade -y

# Qt5 development files
sudo apt install -y \
    qt5-qmake \
    qt5-qmake-bin \
    libqt5core5a \
    libqt5gui5 \
    libqt5widgets5 \
    libqt5network5 \
    libqt5concurrent5 \
    libqt5websockets5

# Qt5 development headers
sudo apt install -y \
    qtbase5-dev \
    qt5-qmake \
    qttools5-dev-tools \
    libqt5websockets5-dev

# Build tools and compilers
sudo apt install -y \
    build-essential \
    gcc \
    g++ \
    make

# CMake (3.16+ compatible with Ubuntu 24)
sudo apt install -y cmake

# Kernel development files (required for kernel module)
sudo apt install -y \
    linux-headers-$(uname -r) \
    linux-headers-generic

# Git (for cloning if needed)
sudo apt install -y git
```

### Verification (Ubuntu 24)

```bash
# Check Qt5 installation
pkg-config --modversion Qt5Core

# Check cmake
cmake --version

# Check gcc/g++
gcc --version && g++ --version

# Check kernel headers
uname -r
ls /lib/modules/$(uname -r)/build
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
cd ~/RealtimeChatApp/ChatDriver/kernel_module
make clean
make
make install
cd ~/RealtimeChatApp/ChatDriver/qt_gui/build
make clean
make
./chatapp
```
Xem log
```bash
cd ~/RealtimeChatApp/ChatDriver/qt_gui/build
cat chatclient.log
```
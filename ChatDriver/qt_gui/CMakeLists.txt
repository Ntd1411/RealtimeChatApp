cmake_minimum_required(VERSION 3.16)
project(ChatApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    Concurrent
    WebSockets
    REQUIRED
)

# Find pthread
find_package(Threads REQUIRED)

# Source files organized by module
set(SOURCES
    src/main.cpp
    src/ui/logindialog.cpp
    src/ui/chatwindow.cpp
    src/api/apiclient.cpp
    src/network/socketclient.cpp
    src/crypto/kernel_crypto_client.cpp
)

set(HEADERS
    src/config.h
    src/ui/logindialog.h
    src/ui/chatwindow.h
    src/api/apiclient.h
    src/network/socketclient.h
    src/crypto/kernel_crypto_client.h
)

# Create executable
add_executable(chatapp ${SOURCES} ${HEADERS})

# Link directories
link_directories(/usr/local/lib)

# Link libraries
target_link_libraries(chatapp
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::WebSockets
    Qt5::Concurrent
    Threads::Threads
)

# Include directories
target_include_directories(chatapp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    /usr/local/include
)

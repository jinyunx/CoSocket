project(CoSocket)

cmake_minimum_required(VERSION 2.6)

include_directories("${PROJECT_SOURCE_DIR}/3rd")
include_directories("${PROJECT_SOURCE_DIR}/net")
include_directories("${PROJECT_SOURCE_DIR}/base")

set(CMAKE_C_FLAGS
    "-Wall -Wextra -Wno-unused-parameter")

set(CMAKE_C_FLAGS_DEBUG
    "-g")

set(CMAKE_C_FLAGS_RELEASE
    "-O3 -g -DRELEASE -DNDEBUG")

set(CMAKE_CXX_FLAGS
    "-Wall -Wextra -Wno-unused-parameter -std=c++11")

set(CMAKE_CXX_FLAGS_DEBUG
    "-g")

set(CMAKE_CXX_FLAGS_RELEASE
    "-O3 -g -DRELEASE -DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS
    "-Wl,-rpath=.:..:lib")

set(CMAKE_USE_RELATIVE_PATHS TRUE)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

### Main library
add_library(CoSocket
    "net/CoSocket.cpp"
    "net/Epoller.cpp"
    "net/Timer.cpp"
    "net/TcpClient.cpp"
    "net/TcpConnector.cpp"
    "net/TcpServer.cpp"
    "3rd/coroutine/coroutine.c"
    )

### Pure coroutine test
add_executable(coroutine_test
    "3rd/coroutine/coroutine_test.c"
    "3rd/coroutine/coroutine.c"
    )

target_link_libraries(coroutine_test
    pthread
    )

### Echo server
add_executable(Echo
    "example/echo/Echo.cpp"
    )

target_link_libraries(Echo
    CoSocket
    )

### Echo client
add_executable(EchoClient
    "example/echo/EchoClient.cpp"
    )

target_link_libraries(EchoClient
    CoSocket
    )

### Signal pipe test
add_executable(SigPipeTest
    "example/sigpipe_test/SigPipeTest.cpp"
    )

target_link_libraries(SigPipeTest
    CoSocket
    )

### Throughput test
add_executable(ThroughputServer
    "example/throughput/ThroughputServer.cpp"
    )

target_link_libraries(ThroughputServer
    CoSocket
    )

add_executable(ThroughputClient
    "example/throughput/ThroughputClient.cpp"
    )

target_link_libraries(ThroughputClient
    CoSocket
    )

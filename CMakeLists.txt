cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 NEW)
project(EdgeIpcDaemon C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS  "Build GoogleTest suite"                 ON)
option(STUB_DBUS    "No-op DaemonBus stub (no libsystemd)"  OFF)
option(ENABLE_ASAN  "AddressSanitizer + UBSan"              OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(edge_ipc_core STATIC
    src/MovingAverageFilter.cpp
    src/ThresholdDetector.cpp
    src/DataProcessor.cpp
    src/SimulatedSource.cpp
    src/FileSource.cpp
    src/TelemetryReceiver.cpp
    src/StateManager.cpp
    src/DaemonBus.cpp
)
target_include_directories(edge_ipc_core PUBLIC include)
target_compile_options(edge_ipc_core PRIVATE -Wall -Wextra)

if(STUB_DBUS)
    target_compile_definitions(edge_ipc_core PUBLIC STUB_DBUS)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDBUS REQUIRED libsystemd)
    target_include_directories(edge_ipc_core PRIVATE ${SDBUS_INCLUDE_DIRS})
    target_link_libraries(edge_ipc_core PUBLIC ${SDBUS_LIBRARIES})
endif()

add_executable(edge_ipc_daemon src/main.cpp)
target_link_libraries(edge_ipc_daemon PRIVATE edge_ipc_core)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

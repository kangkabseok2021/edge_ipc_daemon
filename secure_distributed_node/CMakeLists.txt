# secure_distributed_node/CMakeLists.txt
cmake_minimum_required(VERSION 3.21)
project(SecureDistributedNode CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Standalone build: define edge_ipc_core if the parent hasn't done so ───────
if(NOT TARGET edge_ipc_core)
    add_library(edge_ipc_core STATIC
        ../src/MovingAverageFilter.cpp
        ../src/ThresholdDetector.cpp
        ../src/DataProcessor.cpp
        ../src/SimulatedSource.cpp
        ../src/FileSource.cpp
        ../src/TelemetryReceiver.cpp
        ../src/StateManager.cpp
        ../src/DaemonBus.cpp)
    target_include_directories(edge_ipc_core PUBLIC ../include)
    target_compile_definitions(edge_ipc_core PUBLIC STUB_DBUS)
endif()

# Try CMake config first (macOS Homebrew); fall back to pkg-config (Ubuntu apt)
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
    find_package(Protobuf REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPC     REQUIRED IMPORTED_TARGET grpc++)
    pkg_check_modules(PROTOBUF REQUIRED IMPORTED_TARGET protobuf)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
    # Expose canonical target aliases consumed by the rest of this file
    if(NOT TARGET gRPC::grpc++)
        add_library(gRPC::grpc++ INTERFACE IMPORTED)
        target_link_libraries(gRPC::grpc++ INTERFACE PkgConfig::GRPC)
    endif()
    if(NOT TARGET protobuf::libprotobuf)
        add_library(protobuf::libprotobuf INTERFACE IMPORTED)
        target_link_libraries(protobuf::libprotobuf INTERFACE PkgConfig::PROTOBUF)
    endif()
    # Expose protoc via find_package so Protobuf_PROTOC_EXECUTABLE is set
    find_package(Protobuf QUIET)
    if(NOT Protobuf_PROTOC_EXECUTABLE)
        find_program(Protobuf_PROTOC_EXECUTABLE protoc REQUIRED)
    endif()
    # OpenSSL::SSL / OpenSSL::Crypto are not pulled in transitively via pkg-config
    find_package(OpenSSL REQUIRED)
endif()

# GTest/GMock detection:
# On macOS with Homebrew gRPC, the gRPC package transitively exposes GTest
# headers from /opt/homebrew/include that are ABI-incompatible with
# FetchContent-fetched GTest 1.14.0.  Use the Homebrew GTest directly so
# headers and libs match.  On Linux (CI) use find_package which finds the
# system-installed libgtest-dev package.
if(APPLE AND EXISTS "/opt/homebrew/lib/libgtest.a")
    set(SDN_GTEST_INCLUDE "/opt/homebrew/include")
    set(SDN_GTEST_LIB     "/opt/homebrew/lib/libgtest.a")
    set(SDN_GTEST_MAIN    "/opt/homebrew/lib/libgtest_main.a")
    set(SDN_GMOCK_LIB     "/opt/homebrew/lib/libgmock.a")
    set(SDN_GMOCK_MAIN    "/opt/homebrew/lib/libgmock_main.a")
    set(SDN_GTEST_USE_HOMEBREW TRUE)
else()
    find_package(GTest REQUIRED)
    set(SDN_GTEST_USE_HOMEBREW FALSE)
endif()

# Locate protoc and grpc_cpp_plugin
if(TARGET gRPC::grpc_cpp_plugin)
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
endif()
if(NOT GRPC_CPP_PLUGIN)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
endif()
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(PROTO_FILE "${PROTO_SRC_DIR}/telemetry.proto")

# Generate C++ protobuf + gRPC stubs
add_custom_command(
    OUTPUT
        "${PROTO_GEN_DIR}/telemetry.pb.cc"
        "${PROTO_GEN_DIR}/telemetry.pb.h"
        "${PROTO_GEN_DIR}/telemetry.grpc.pb.cc"
        "${PROTO_GEN_DIR}/telemetry.grpc.pb.h"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        --proto_path=${PROTO_SRC_DIR}
        --cpp_out=${PROTO_GEN_DIR}
        --grpc_out=${PROTO_GEN_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC/Protobuf stubs from telemetry.proto"
)

# A small library for the generated stubs (reused by both exe and tests)
add_library(telemetry_proto STATIC
    "${PROTO_GEN_DIR}/telemetry.pb.cc"
    "${PROTO_GEN_DIR}/telemetry.grpc.pb.cc"
)
target_include_directories(telemetry_proto PUBLIC "${PROTO_GEN_DIR}")
target_link_libraries(telemetry_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)
target_compile_definitions(telemetry_proto PUBLIC STUB_DBUS)

# ── telemetry_node executable ─────────────────────────────────────────────────
add_executable(telemetry_node
    src/TelemetryNode.cpp
    src/TlsCredentials.cpp
    src/main_node.cpp
)
target_include_directories(telemetry_node PRIVATE
    include
    "${CMAKE_SOURCE_DIR}/include"   # parent project headers
    "${PROTO_GEN_DIR}"
)
target_link_libraries(telemetry_node PRIVATE
    edge_ipc_core
    telemetry_proto
    gRPC::grpc++
    protobuf::libprotobuf
)
target_compile_definitions(telemetry_node PRIVATE STUB_DBUS)

# ── sdn_tests executable ──────────────────────────────────────────────────────
add_executable(sdn_tests
    src/TelemetryNode.cpp
    src/TlsCredentials.cpp
    tests/test_telemetry_node.cpp
    tests/test_tls_credentials.cpp
)
target_include_directories(sdn_tests PRIVATE
    include
    "${CMAKE_SOURCE_DIR}/include"
    "${PROTO_GEN_DIR}"
)
if(SDN_GTEST_USE_HOMEBREW)
    # Homebrew GTest: prepend header path so it wins over any other GTest headers
    target_include_directories(sdn_tests BEFORE PRIVATE "${SDN_GTEST_INCLUDE}")
    target_link_libraries(sdn_tests PRIVATE
        edge_ipc_core
        telemetry_proto
        gRPC::grpc++
        protobuf::libprotobuf
        "${SDN_GMOCK_MAIN}"
        "${SDN_GMOCK_LIB}"
        "${SDN_GTEST_MAIN}"
        "${SDN_GTEST_LIB}"
        pthread
    )
else()
    # System GTest (Linux / CI)
    target_link_libraries(sdn_tests PRIVATE
        edge_ipc_core
        telemetry_proto
        gRPC::grpc++
        protobuf::libprotobuf
        GTest::gmock_main
        GTest::gmock
        GTest::gtest_main
        GTest::gtest
        pthread
    )
endif()
target_compile_definitions(sdn_tests PRIVATE
    STUB_DBUS
    TEST_CERTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/test_certs"
)

include(GoogleTest)
add_test(NAME sdn_tests COMMAND sdn_tests)
set_tests_properties(sdn_tests PROPERTIES TIMEOUT 60)

# ── telemetry_coordinator executable ─────────────────────────────────────────
add_executable(telemetry_coordinator
    src/TelemetryCoordinator.cpp
    src/TlsCredentials.cpp
    src/main_coordinator.cpp)
target_include_directories(telemetry_coordinator PRIVATE
    include
    "${CMAKE_CURRENT_BINARY_DIR}")
# On macOS with Homebrew, add the Homebrew include path for httplib/nlohmann
if(APPLE AND EXISTS "/opt/homebrew/include")
    target_include_directories(telemetry_coordinator PRIVATE /opt/homebrew/include)
endif()
target_compile_definitions(telemetry_coordinator PRIVATE STUB_DBUS)
target_link_libraries(telemetry_coordinator PRIVATE
    telemetry_proto
    gRPC::grpc++ protobuf::libprotobuf OpenSSL::SSL OpenSSL::Crypto)

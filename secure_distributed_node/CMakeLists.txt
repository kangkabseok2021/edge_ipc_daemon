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

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

# gRPC (from Homebrew) transitively exposes GTest 1.17.0 headers via
# /opt/homebrew/include, which are ABI-incompatible with the fetched
# GTest 1.14.0 that CMake's GTest::* targets point to on this machine.
# Detect the mismatch and use Homebrew GTest directly so headers + libs match.
set(SDN_GTEST_INCLUDE "/opt/homebrew/include")
set(SDN_GTEST_LIB     "/opt/homebrew/lib/libgtest.a")
set(SDN_GTEST_MAIN    "/opt/homebrew/lib/libgtest_main.a")
set(SDN_GMOCK_LIB     "/opt/homebrew/lib/libgmock.a")
set(SDN_GMOCK_MAIN    "/opt/homebrew/lib/libgmock_main.a")

# Locate protoc and grpc_cpp_plugin
get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
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
    "${SDN_GTEST_INCLUDE}"   # ensure Homebrew GTest 1.17.0 headers come first
)
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
target_compile_definitions(sdn_tests PRIVATE
    STUB_DBUS
    TEST_CERTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/test_certs"
)

include(GoogleTest)
# gtest_discover_tests requires the GTest CMake target to correctly install
# the post-build discovery script.  Since sdn_tests links raw Homebrew .a
# files instead of GTest::* targets, we fall back to add_test with the
# --gtest_output flag which is perfectly understood by ctest.
add_test(NAME sdn_tests COMMAND sdn_tests)
set_tests_properties(sdn_tests PROPERTIES TIMEOUT 60)

# ── telemetry_coordinator executable ─────────────────────────────────────────
add_executable(telemetry_coordinator
    src/TelemetryCoordinator.cpp
    src/TlsCredentials.cpp
    src/main_coordinator.cpp)
target_include_directories(telemetry_coordinator PRIVATE
    include
    "${CMAKE_CURRENT_BINARY_DIR}"
    /opt/homebrew/include)   # httplib + nlohmann/json
target_compile_definitions(telemetry_coordinator PRIVATE STUB_DBUS)
target_link_libraries(telemetry_coordinator PRIVATE
    telemetry_proto
    gRPC::grpc++ protobuf::libprotobuf OpenSSL::SSL OpenSSL::Crypto)

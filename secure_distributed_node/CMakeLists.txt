# secure_distributed_node/CMakeLists.txt
# No project() or cmake_minimum_required() — included as a subdirectory

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

cmake_minimum_required(VERSION 3.21)
project(AsyncTelemetryServer VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Our strict warning set — applied ONLY to first-party targets (PRIVATE)
# so that FetchContent dependencies (gtest, jwt-cpp) are not affected.
set(ATS_WARNINGS
    -Wall -Wextra -Wpedantic -Werror
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    -Woverloaded-virtual -Wcast-align -Wunused
    -Wconversion -Wsign-conversion -Wnull-dereference
    -Wdouble-promotion -Wformat=2
)

# Homebrew paths on macOS
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/openssl@3")
    # Ensure pkg-config can locate libpq (required by libpqxx)
    set(ENV{PKG_CONFIG_PATH}
        "/opt/homebrew/lib/pkgconfig:/opt/homebrew/opt/libpq/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif()

# Required dependencies
# Boost 1.74+: boost_system is header-only and merged into Boost::headers
find_package(Boost 1.74 REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)
FetchContent_Declare(jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG        v0.7.0
    GIT_SHALLOW    TRUE)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JWT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jwt-cpp)

# libpqxx — try config first, fall back to pkg-config
find_package(libpqxx QUIET)
if(NOT libpqxx_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)
    add_library(libpqxx::pqxx ALIAS PkgConfig::PQXX)
endif()

# ASan + UBSan
option(ENABLE_ASAN "Build with AddressSanitizer + UBSan" OFF)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Core library (everything except main + client)
add_library(ats_core STATIC
    src/FrameProtocol.cpp
    src/JwtValidator.cpp
    src/DbPool.cpp
    src/TelemetryRepository.cpp
    src/TcpServer.cpp
    src/TlsServer.cpp
)
target_include_directories(ats_core PUBLIC include)
target_compile_options(ats_core PRIVATE ${ATS_WARNINGS})
target_link_libraries(ats_core PUBLIC
    Boost::headers
    OpenSSL::SSL
    OpenSSL::Crypto
    jwt-cpp::jwt-cpp
    libpqxx::pqxx
)

# Server executable
add_executable(telemetry_server src/main.cpp)
target_compile_options(telemetry_server PRIVATE ${ATS_WARNINGS})
target_link_libraries(telemetry_server PRIVATE ats_core)

# Client executable
add_executable(telemetry_client src/TelemetryClient.cpp)
target_compile_options(telemetry_client PRIVATE ${ATS_WARNINGS})
target_link_libraries(telemetry_client PRIVATE ats_core)

# Tests
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    # Prefer system-installed GTest (avoids ABI mismatch with FetchContent version)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
            GIT_SHALLOW    TRUE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
    add_executable(ats_tests
        tests/test_protocol.cpp
        tests/test_jwt.cpp
    )
    target_compile_options(ats_tests PRIVATE ${ATS_WARNINGS})
    target_link_libraries(ats_tests PRIVATE ats_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(ats_tests)
endif()

# Clang-Tidy target
find_program(CLANG_TIDY_EXE clang-tidy
    HINTS "/opt/homebrew/opt/llvm/bin")
if(CLANG_TIDY_EXE)
    set(TIDY_SOURCES
        ${CMAKE_SOURCE_DIR}/src/FrameProtocol.cpp
        ${CMAKE_SOURCE_DIR}/src/JwtValidator.cpp
        ${CMAKE_SOURCE_DIR}/src/TcpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/TlsServer.cpp
    )
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} ${TIDY_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

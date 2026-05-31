cmake_minimum_required(VERSION 3.20)
project(opcua_edge_server C CXX)

set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS             "Build GoogleTest suite"                      ON)
option(ENABLE_ASAN             "AddressSanitizer + UBSan"                    OFF)
option(USE_OPTIMISED_DATASOURCE "Use pre-allocated UA_Variant pool (Phase 4)" OFF)

# ── open62541 v1.3.9 via FetchContent (amalgamated single-file build) ─────────
include(FetchContent)

set(UA_ENABLE_AMALGAMATION  ON  CACHE BOOL "" FORCE)
set(UA_BUILD_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(UA_BUILD_UNIT_TESTS     OFF CACHE BOOL "" FORCE)
set(UA_LOGLEVEL             400 CACHE STRING "" FORCE)  # errors only in tests
set(BUILD_SHARED_LIBS       OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    open62541
    GIT_REPOSITORY https://github.com/open62541/open62541.git
    GIT_TAG        v1.3.9
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   FALSE
)
FetchContent_MakeAvailable(open62541)

# ── CNC core static library ───────────────────────────────────────────────────
if(USE_OPTIMISED_DATASOURCE)
    set(DATASOURCE_SRC src/data_source_opt.c)
else()
    set(DATASOURCE_SRC src/data_source.c)
endif()

add_library(cnc_core STATIC
    src/telemetry_sim.c
    src/information_model.c
    src/update_loop.c
    ${DATASOURCE_SRC}
)
target_include_directories(cnc_core PUBLIC src)
target_link_libraries(cnc_core PUBLIC open62541)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(cnc_core PRIVATE rt pthread m)
else()
    target_link_libraries(cnc_core PRIVATE m)
endif()
if(ENABLE_ASAN)
    target_compile_options(cnc_core PUBLIC
        -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(cnc_core PUBLIC -fsanitize=address,undefined)
endif()

# ── Server binary ─────────────────────────────────────────────────────────────
add_executable(cnc_server src/server_main.c)
target_link_libraries(cnc_server PRIVATE cnc_core)
if(ENABLE_ASAN)
    target_compile_options(cnc_server PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(cnc_server PRIVATE -fsanitize=address,undefined)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()

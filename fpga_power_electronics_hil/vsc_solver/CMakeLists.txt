cmake_minimum_required(VERSION 3.20)
project(vsc_solver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vsc_solver_core STATIC
    src/LuSolver.cpp
    src/NortonUpdate.cpp
    src/VscNetwork.cpp
)
target_include_directories(vsc_solver_core PUBLIC include)
target_compile_options(vsc_solver_core PRIVATE
    $<IF:$<CXX_COMPILER_ID:MSVC>,/W3,-Wall -Wextra>
)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
    DOWNLOAD_EXTRACT_TIMESTAMP NEW
)
set(INSTALL_GTEST OFF)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
if(NOT TARGET GTest::gtest_main)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()
add_executable(test_vsc_solver tests/test_vsc_solver.cpp)
target_link_libraries(test_vsc_solver PRIVATE vsc_solver_core GTest::gtest_main)
target_compile_options(test_vsc_solver PRIVATE
    $<IF:$<CXX_COMPILER_ID:MSVC>,/W3,-Wall -Wextra>
)
add_test(NAME vsc_solver_tests COMMAND test_vsc_solver)

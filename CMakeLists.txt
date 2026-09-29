cmake_minimum_required(VERSION 3.18)
project(branchless_gf128_simd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(gf128 src/reference.cpp)
target_include_directories(gf128 PUBLIC include)
target_compile_options(gf128 PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
  add_executable(test_reference tests/test_reference.cpp)
  target_link_libraries(test_reference PRIVATE gf128)
  add_test(NAME reference COMMAND test_reference)
endif()

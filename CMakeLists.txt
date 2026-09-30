cmake_minimum_required(VERSION 3.18)
project(branchless_gf128_simd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(gf128 src/reference.cpp src/matrix.cpp src/kernel_avx512.cpp)
# Only the kernel gets the ISA flags, so everything else still runs on any x86-64.
set_source_files_properties(src/kernel_avx512.cpp PROPERTIES
  COMPILE_OPTIONS "-mavx512f;-mavx512bw;-mgfni;-mvpclmulqdq")
target_include_directories(gf128 PUBLIC include)
target_compile_options(gf128 PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
  # Path to Intel SDE's sde64 to run the kernel test on CPUs without AVX-512.
  set(GF128_SDE "" CACHE FILEPATH "Intel SDE sde64 binary")
  foreach(t reference matrix affine kernel)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE gf128)
    if(t STREQUAL "kernel" AND GF128_SDE)
      add_test(NAME ${t} COMMAND ${GF128_SDE} -spr -- $<TARGET_FILE:test_${t}>)
    else()
      add_test(NAME ${t} COMMAND test_${t})
    endif()
  endforeach()
  set_tests_properties(kernel PROPERTIES SKIP_RETURN_CODE 77)
endif()

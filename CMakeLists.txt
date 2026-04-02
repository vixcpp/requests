cmake_minimum_required(VERSION 3.20)
project(requests VERSION 0.1.0 LANGUAGES CXX)

add_library(requests INTERFACE)
add_library(requests::requests ALIAS requests)

target_compile_features(requests INTERFACE cxx_std_20)

target_include_directories(requests
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(MSVC)
  target_compile_options(requests INTERFACE /W4 /permissive-)
else()
  target_compile_options(requests INTERFACE -Wall -Wextra -Wpedantic)
endif()

option(REQUESTS_BUILD_TESTS "Build requests tests" OFF)
option(REQUESTS_BUILD_EXAMPLES "Build requests examples" OFF)

include(CTest)

if(REQUESTS_BUILD_TESTS)
  enable_testing()

  add_executable(requests_test_basic tests/test_basic.cpp)
  target_link_libraries(requests_test_basic PRIVATE requests::requests)

  add_test(NAME requests.basic COMMAND requests_test_basic)
endif()

if(REQUESTS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

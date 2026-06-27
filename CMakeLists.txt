#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/requests
#  Use of this source code is governed by a MIT license
#  that can be found in the LICENSE file.
#
# ====================================================================
# Vix Requests
# ====================================================================

cmake_minimum_required(VERSION 3.20)

set(VIX_REQUESTS_MODULE_VERSION 1.0.0)

# ============================================================================
# Re-entry guard
# ============================================================================

get_property(_VIX_REQUESTS_MODULE_ALREADY_INCLUDED GLOBAL PROPERTY VIX_REQUESTS_MODULE_ALREADY_INCLUDED)

if(_VIX_REQUESTS_MODULE_ALREADY_INCLUDED)
  return()
endif()

set_property(GLOBAL PROPERTY VIX_REQUESTS_MODULE_ALREADY_INCLUDED TRUE)

# ============================================================================
# Standalone vs Umbrella
# ============================================================================

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(vix_requests
    VERSION ${VIX_REQUESTS_MODULE_VERSION}
    DESCRIPTION "Simple HTTP requests module for Vix.cpp"
    LANGUAGES CXX
  )
endif()

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

# ============================================================================
# Options
# ============================================================================

option(VIX_REQUESTS_BUILD_TESTS "Build Vix Requests tests" ON)
option(VIX_REQUESTS_BUILD_EXAMPLES "Build Vix Requests examples" ON)
option(VIX_REQUESTS_ENABLE_LTO "Enable LTO in Release builds" OFF)
option(VIX_REQUESTS_INSTALL "Enable standalone install/package rules" ON)

if(DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_REQUESTS_INSTALL OFF CACHE BOOL "Enable standalone install/package rules" FORCE)
endif()

# ============================================================================
# C++ standard
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Sources
# ============================================================================

set(VIX_REQUESTS_SOURCES
  src/Error.cpp
  src/Method.cpp
  src/Headers.cpp
  src/Params.cpp
  src/Url.cpp
  src/Body.cpp
  src/Timeout.cpp
  src/RequestOptions.cpp
  src/Request.cpp
  src/Response.cpp
  src/Client.cpp
  src/Session.cpp
  src/Version.cpp

  src/detail/Base64.cpp
  src/detail/CaseInsensitive.cpp
  src/detail/UrlEncode.cpp

  src/http/HttpParser.cpp
  src/http/HttpSerializer.cpp
  src/http/RedirectPolicy.cpp
  src/http/CookieJar.cpp

  src/transport/Resolver.cpp
  src/transport/Socket.cpp
  src/transport/TcpTransport.cpp
  src/transport/HttpsTransport.cpp
  src/transport/TransportFactory.cpp
)

# ============================================================================
# Library
# ============================================================================

add_library(vix_requests STATIC ${VIX_REQUESTS_SOURCES})

if(NOT TARGET vix::requests)
  add_library(vix::requests ALIAS vix_requests)
endif()

target_include_directories(vix_requests
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(vix_requests PUBLIC cxx_std_20)

if(NOT TARGET vix::async)
  find_package(async CONFIG REQUIRED)
endif()

find_package(OpenSSL REQUIRED)

target_link_libraries(vix_requests
  PUBLIC
    vix::async
  PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

set_target_properties(vix_requests PROPERTIES
  OUTPUT_NAME vix_requests
  VERSION ${VIX_REQUESTS_MODULE_VERSION}
  SOVERSION 1
  EXPORT_NAME requests
)

# ============================================================================
# Warnings
# ============================================================================

function(vix_requests_add_flag_if_supported target flag)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" flag_var "${flag}")
  set(test_var "HAVE_VIX_REQUESTS_${flag_var}")

  check_cxx_compiler_flag("${flag}" ${test_var})

  if(${test_var})
    target_compile_options(${target} PRIVATE "${flag}")
  endif()
endfunction()

if(MSVC)
  vix_requests_add_flag_if_supported(vix_requests /W4)
  vix_requests_add_flag_if_supported(vix_requests /permissive-)
else()
  vix_requests_add_flag_if_supported(vix_requests -Wall)
  vix_requests_add_flag_if_supported(vix_requests -Wextra)
  vix_requests_add_flag_if_supported(vix_requests -Wshadow)
  vix_requests_add_flag_if_supported(vix_requests -Wconversion)
  vix_requests_add_flag_if_supported(vix_requests -Wformat=2)
  vix_requests_add_flag_if_supported(vix_requests -Wpedantic)
endif()

if(TARGET vix_warnings)
  target_link_libraries(vix_requests PRIVATE $<BUILD_INTERFACE:vix_warnings>)
endif()

if(VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
  target_link_libraries(vix_requests PRIVATE vix_sanitizers)
endif()

# ============================================================================
# Version string
# ============================================================================

set(_VIX_REQUESTS_VERSION "v${VIX_REQUESTS_MODULE_VERSION}")

if(DEFINED VIX_REQUESTS_VERSION AND NOT "${VIX_REQUESTS_VERSION}" STREQUAL "")
  set(_VIX_REQUESTS_VERSION "${VIX_REQUESTS_VERSION}")

elseif(DEFINED ENV{VIX_REQUESTS_VERSION} AND NOT "$ENV{VIX_REQUESTS_VERSION}" STREQUAL "")
  set(_VIX_REQUESTS_VERSION "$ENV{VIX_REQUESTS_VERSION}")

elseif(DEFINED VIX_UMBRELLA_VERSION AND NOT "${VIX_UMBRELLA_VERSION}" STREQUAL "")
  set(_VIX_REQUESTS_VERSION "${VIX_UMBRELLA_VERSION}")
endif()

message(STATUS "Vix Requests version: ${_VIX_REQUESTS_VERSION}")

target_compile_definitions(vix_requests
  PRIVATE
    VIX_REQUESTS_VERSION="${_VIX_REQUESTS_VERSION}"
)

# ============================================================================
# LTO
# ============================================================================

if(VIX_REQUESTS_ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)

  check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_msg)

  if(_ipo_ok)
    set_property(TARGET vix_requests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "[requests] IPO/LTO not supported: ${_ipo_msg}")
  endif()
endif()

# ============================================================================
# Tests
# ============================================================================

if(VIX_REQUESTS_BUILD_TESTS)
  enable_testing()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  else()
    message(STATUS "[requests] tests/ missing CMakeLists.txt -> skipping")
  endif()
endif()

# ============================================================================
# Examples
# ============================================================================

if(VIX_REQUESTS_BUILD_EXAMPLES)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
  else()
    message(STATUS "[requests] examples/ missing CMakeLists.txt -> skipping")
  endif()
endif()

# ============================================================================
# Install for Vix umbrella export
# ============================================================================

if(DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD AND VIX_ENABLE_INSTALL)
  install(
    TARGETS vix_requests
    EXPORT VixTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# ============================================================================
# Standalone install/package
# ============================================================================

if(VIX_REQUESTS_INSTALL AND NOT (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD))
  include(CMakePackageConfigHelpers)

  install(
    TARGETS vix_requests
    EXPORT vix_requestsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    EXPORT vix_requestsTargets
    FILE vix_requestsTargets.cmake
    NAMESPACE vix::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vix_requests
  )

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vix_requestsConfig.cmake.in")
    configure_package_config_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/vix_requestsConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/vix_requestsConfig.cmake
      INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vix_requests
    )

    write_basic_package_version_file(
      ${CMAKE_CURRENT_BINARY_DIR}/vix_requestsConfigVersion.cmake
      VERSION ${VIX_REQUESTS_MODULE_VERSION}
      COMPATIBILITY SameMajorVersion
    )

    install(
      FILES
        ${CMAKE_CURRENT_BINARY_DIR}/vix_requestsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/vix_requestsConfigVersion.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vix_requests
    )
  endif()
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "------------------------------------------------------")
message(STATUS "vix::requests configured (${VIX_REQUESTS_MODULE_VERSION})")
message(STATUS "Mode: STATIC")
message(STATUS "HTTPS transport: OpenSSL")
message(STATUS "Include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Internal dir: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Build tests: ${VIX_REQUESTS_BUILD_TESTS}")
message(STATUS "Build examples: ${VIX_REQUESTS_BUILD_EXAMPLES}")
message(STATUS "Install: ${VIX_REQUESTS_INSTALL}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "------------------------------------------------------")

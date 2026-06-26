#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/requests
#  Use of this source code is governed by a MIT license
#  that can be found in the LICENSE file.
#
# ====================================================================
# Vix Requests tests
# ====================================================================

cmake_minimum_required(VERSION 3.20)

include(CTest)

find_package(Threads REQUIRED)

# ============================================================================
# Helpers
# ============================================================================

function(vix_requests_add_test test_name test_source)
  set(test_target "vix_requests_test_${test_name}")

  if(NOT EXISTS "${test_source}")
    message(WARNING "[requests/tests] Missing test source: ${test_source}")
    return()
  endif()

  add_executable(${test_target} "${test_source}")

  target_link_libraries(${test_target}
    PRIVATE
      vix::requests
      Threads::Threads
  )

  target_include_directories(${test_target}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/integration
      ${CMAKE_CURRENT_SOURCE_DIR}/../src
  )

  target_compile_features(${test_target}
    PRIVATE
      cxx_std_20
  )

  if(TARGET vix_warnings)
    target_link_libraries(${test_target}
      PRIVATE
        vix_warnings
    )
  endif()

  set_target_properties(${test_target} PROPERTIES
    OUTPUT_NAME "${test_name}"
  )

  add_test(
    NAME ${test_name}
    COMMAND ${test_target}
  )
endfunction()

# ============================================================================
# Unit tests
# ============================================================================

vix_requests_add_test(
  url_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/url_test.cpp"
)

vix_requests_add_test(
  headers_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/headers_test.cpp"
)

vix_requests_add_test(
  params_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/params_test.cpp"
)

vix_requests_add_test(
  body_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/body_test.cpp"
)

vix_requests_add_test(
  response_parser_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/response_parser_test.cpp"
)

vix_requests_add_test(
  serializer_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/serializer_test.cpp"
)

vix_requests_add_test(
  session_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/session_test.cpp"
)

vix_requests_add_test(
  errors_test
  "${CMAKE_CURRENT_SOURCE_DIR}/unit/errors_test.cpp"
)

# ============================================================================
# Integration tests
# ============================================================================

vix_requests_add_test(
  http_client_test
  "${CMAKE_CURRENT_SOURCE_DIR}/integration/http_client_test.cpp"
)

vix_requests_add_test(
  redirect_test
  "${CMAKE_CURRENT_SOURCE_DIR}/integration/redirect_test.cpp"
)

vix_requests_add_test(
  timeout_test
  "${CMAKE_CURRENT_SOURCE_DIR}/integration/timeout_test.cpp"
)

message(STATUS "[requests/tests] Configured Vix Requests tests.")

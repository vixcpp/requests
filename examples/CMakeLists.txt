#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira. All rights reserved.
#  https://github.com/vixcpp/requests
#  Use of this source code is governed by a MIT license
#  that can be found in the LICENSE file.
#
# ====================================================================
# Vix Requests examples
# ====================================================================

cmake_minimum_required(VERSION 3.20)

set(VIX_REQUESTS_EXAMPLES
  # Basic examples
  simple_get
  get_params
  post_json
  post_form
  session
  error_handling

  # Numbered production-style examples
  01_get_with_params
  02_post_json
  03_form_login_session
  04_bearer_token
  05_download_file
  06_error_handling
  07_timeout_retry
  08_async_get
  09_paginated_api
  10_api_client_wrapper
)

foreach(example_name IN LISTS VIX_REQUESTS_EXAMPLES)
  set(example_source "${CMAKE_CURRENT_SOURCE_DIR}/${example_name}.cpp")
  set(example_target "vix_requests_example_${example_name}")

  if(EXISTS "${example_source}")
    add_executable(${example_target} "${example_source}")

    target_link_libraries(${example_target}
      PRIVATE
        vix::requests
    )

    target_compile_features(${example_target}
      PRIVATE
        cxx_std_20
    )

    target_include_directories(${example_target}
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(TARGET vix_warnings)
      target_link_libraries(${example_target}
        PRIVATE
          vix_warnings
      )
    endif()

    set_target_properties(${example_target} PROPERTIES
      OUTPUT_NAME "${example_name}"
    )
  else()
    message(WARNING "[requests/examples] Missing example source: ${example_source}")
  endif()
endforeach()

message(STATUS "[requests/examples] Configured Vix Requests examples.")

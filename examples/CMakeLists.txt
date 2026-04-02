function(requests_add_example target file)
  add_executable(${target} ${file})
  target_link_libraries(${target} PRIVATE requests::requests)
endfunction()

requests_add_example(requests_example_get 01_get.cpp)
requests_add_example(requests_example_post 02_post.cpp)
requests_add_example(requests_example_headers 03_headers.cpp)
requests_add_example(requests_example_json 04_json.cpp)

/**
 * @file 02_post.cpp
 * @brief Basic POST request example
 */

#include <iostream>
#include <vix/requests/requests.hpp>

int main()
{
  try
  {
    vix::requests::RequestOptions options;
    options.body = "name=Gaspard&role=builder";
    options.headers["Content-Type"] = "application/x-www-form-urlencoded";

    auto response = vix::requests::post("https://httpbin.org/post", options);

    std::cout << "Status: " << response.status_code << '\n';
    std::cout << "Body:\n"
              << response.text << '\n';

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Request failed: " << e.what() << '\n';
    return 1;
  }
}

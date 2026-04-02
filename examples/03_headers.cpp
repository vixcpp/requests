/**
 * @file 03_headers.cpp
 * @brief Custom headers example
 */

#include <iostream>
#include <vix/requests/requests.hpp>

int main()
{
  try
  {
    vix::requests::RequestOptions options;
    options.headers["Accept"] = "application/json";
    options.headers["X-App"] = "Vix Requests";
    options.headers["X-Example"] = "headers";

    auto response = vix::requests::get("https://httpbin.org/headers", options);

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

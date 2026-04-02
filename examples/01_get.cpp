/**
 * @file 01_get.cpp
 * @brief Basic GET request example
 */

#include <iostream>
#include <vix/requests/requests.hpp>

int main()
{
  try
  {
    auto response = vix::requests::get("https://httpbin.org/get");

    std::cout << "Status: " << response.status_code << '\n';
    std::cout << "OK: " << std::boolalpha << response.ok() << '\n';
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

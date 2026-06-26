/**
 *
 *  @file simple_get.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Simple GET example for Vix Requests.
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/requests
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix Requests
 *
 */

#include <vix/requests/requests.hpp>

#include <iostream>

int main()
{
  try
  {
    const auto response =
        vix::requests::get("http://example.com/");

    std::cout << "Status : " << response.status_code() << '\n';
    std::cout << "Reason : " << response.reason() << '\n';
    std::cout << "URL    : " << response.url() << '\n';
    std::cout << "OK     : " << (response.ok() ? "yes" : "no") << '\n';
    std::cout << '\n';

    response.raise_for_status();

    std::cout << response.text() << '\n';

    return 0;
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request error: " << error.what() << '\n';
    return 1;
  }
}

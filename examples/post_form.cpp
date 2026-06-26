/**
 *
 *  @file post_form.cpp
 *  @author Gaspard Kirira
 *
 *  @brief POST form example for Vix Requests.
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

#include <chrono>
#include <iostream>

int main()
{
  try
  {
    vix::requests::RequestOptions options;

    options.headers.set("Accept", "text/plain");
    options.timeout = std::chrono::seconds(10);

    const auto response =
        vix::requests::post(
            "http://127.0.0.1:8080/login",
            vix::requests::form_body({{"username", "gaspard"},
                                      {"project", "Vix Requests"}}),
            options);

    std::cout << "Status : " << response.status_code() << '\n';
    std::cout << "Reason : " << response.reason() << '\n';
    std::cout << "URL    : " << response.url() << '\n';
    std::cout << "Size   : " << response.size() << " bytes\n";
    std::cout << '\n';

    response.raise_for_status();

    std::cout << response.text() << '\n';

    return 0;
  }
  catch (const vix::requests::TimeoutException &error)
  {
    std::cerr << "timeout: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request error: " << error.what() << '\n';
    return 1;
  }
}

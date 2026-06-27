/**
 *
 *  @file post_json.cpp
 *  @author Gaspard Kirira
 *
 *  @brief POST JSON example for Vix Requests.
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
#include "example_server.hpp"

#include <chrono>
#include <iostream>

int main()
{
  try
  {
    vix_examples::requests::ExampleHttpServer server;
    vix::requests::RequestOptions options;

    options.headers.set("Accept", "application/json");
    options.timeout = std::chrono::seconds(10);

    const auto response =
        vix::requests::post(
            server.url("/api/items"),
            vix::requests::json_body(R"({"name":"Vix","type":"requests"})"),
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

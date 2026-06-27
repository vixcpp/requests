/**
 *
 *  @file session.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Session example for Vix Requests.
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
    vix::requests::Session session;

    session.headers().set("User-Agent", "vix-requests-example/1.0.0");
    session.headers().set("Accept", "application/json");

    session.params().set("token", "demo-token");

    session.timeout() = std::chrono::seconds(10);

    const auto first =
        session.get(server.url("/api/profile"));

    std::cout << "First status : " << first.status_code() << '\n';
    std::cout << "First URL    : " << first.url() << '\n';
    std::cout << '\n';

    first.raise_for_status();

    std::cout << first.text() << "\n\n";

    const auto second =
        session.post(
            server.url("/api/items"),
            vix::requests::form_body({{"name", "Gaspard"},
                                      {"project", "Vix Requests"}}));

    std::cout << "Second status : " << second.status_code() << '\n';
    std::cout << "Second URL    : " << second.url() << '\n';
    std::cout << '\n';

    second.raise_for_status();

    std::cout << second.text() << '\n';

    return 0;
  }
  catch (const vix::requests::TimeoutException &error)
  {
    std::cerr << "timeout: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::UnsupportedProtocolException &error)
  {
    std::cerr << "unsupported protocol: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::HttpException &error)
  {
    std::cerr << "HTTP error: "
              << error.status_code()
              << " "
              << error.reason()
              << " "
              << error.url()
              << '\n';

    return 1;
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request error: " << error.what() << '\n';
    return 1;
  }
}

/**
 *
 *  @file error_handling.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Error handling example for Vix Requests.
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

    options.timeout = std::chrono::seconds(5);
    options.max_redirects = 5;
    options.follow_redirects = true;

    const auto response =
        vix::requests::get(
            server.url("/missing"),
            options);

    std::cout << "Status : " << response.status_code() << '\n';
    std::cout << "Reason : " << response.reason() << '\n';
    std::cout << "URL    : " << response.url() << '\n';
    std::cout << '\n';

    response.raise_for_status();

    std::cout << response.text() << '\n';

    return 0;
  }
  catch (const vix::requests::InvalidUrlException &error)
  {
    std::cerr << "invalid URL: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::UnsupportedProtocolException &error)
  {
    std::cerr << "unsupported protocol: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::TimeoutException &error)
  {
    std::cerr << "timeout: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::ConnectionException &error)
  {
    std::cerr << "connection error: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::TooManyRedirectsException &error)
  {
    std::cerr << "redirect error: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::HttpException &error)
  {
    std::cerr << "HTTP error: "
              << error.status_code()
              << " "
              << error.reason();

    if (!error.url().empty())
    {
      std::cerr << " for " << error.url();
    }

    std::cerr << '\n';
    return 0;
  }
  catch (const vix::requests::TransportException &error)
  {
    std::cerr << "transport error: " << error.what() << '\n';
    return 1;
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request error: " << error.what() << '\n';
    return 1;
  }
}

/**
 *
 *  @file timeout_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Integration tests for Vix Requests timeouts.
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

#include "local_http_server.hpp"

#include <vix/requests/requests.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
  void expect(bool condition, const char *message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_fast_response_with_timeout()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &)
        {
          return vix::requests::tests::local_http_response(
              200,
              "fast");
        });

    vix::requests::RequestOptions options;
    options.timeout = std::chrono::seconds(2);

    const auto response =
        vix::requests::get(server.url("/fast"), options);

    expect(response.status_code() == 200, "fast response should return 200");
    expect(response.text() == "fast", "fast response body should match");
    expect(response.elapsed().count() >= 0, "elapsed time should be valid");
  }

  void test_read_timeout()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(250));

          return vix::requests::tests::local_http_response(
              200,
              "slow");
        });

    vix::requests::RequestOptions options;
    options.timeout.set_connect(std::chrono::seconds(2));
    options.timeout.set_read(std::chrono::milliseconds(50));
    options.timeout.set_total(std::chrono::seconds(2));

    bool thrown = false;

    try
    {
      static_cast<void>(vix::requests::get(server.url("/slow"), options));
    }
    catch (const vix::requests::TimeoutException &)
    {
      thrown = true;
    }

    expect(thrown, "slow response should trigger read timeout");
  }

  void test_tiny_timeout_object()
  {
    const auto timeout = vix::requests::Timeout::milliseconds(10);

    expect(timeout.active(), "timeout should be active");
    expect(timeout.has_connect(), "connect timeout should be active");
    expect(timeout.has_read(), "read timeout should be active");
    expect(timeout.has_total(), "total timeout should be active");
    expect(timeout.connect() == std::chrono::milliseconds(10),
           "connect timeout should be 10ms");
    expect(timeout.read() == std::chrono::milliseconds(10),
           "read timeout should be 10ms");
    expect(timeout.total() == std::chrono::milliseconds(10),
           "total timeout should be 10ms");
  }

  void test_timeout_none()
  {
    const auto timeout = vix::requests::Timeout::none();

    expect(!timeout.active(), "none timeout should not be active");
    expect(!timeout.has_connect(), "none timeout should not have connect");
    expect(!timeout.has_read(), "none timeout should not have read");
    expect(!timeout.has_total(), "none timeout should not have total");
  }

  void test_negative_timeout_throws()
  {
    bool thrown = false;

    try
    {
      static_cast<void>(vix::requests::Timeout::milliseconds(-1));
    }
    catch (const vix::requests::RequestException &)
    {
      thrown = true;
    }

    expect(thrown, "negative timeout should throw");
  }
}

int main()
{
  try
  {
    test_fast_response_with_timeout();
    test_read_timeout();
    test_tiny_timeout_object();
    test_timeout_none();
    test_negative_timeout_throws();

    std::cout << "timeout_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "timeout_test failed: " << error.what() << '\n';
    return 1;
  }
}

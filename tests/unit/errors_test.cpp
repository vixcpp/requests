/**
 *
 *  @file errors_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests errors.
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

#include <vix/requests/Error.hpp>
#include <vix/requests/Response.hpp>
#include <vix/requests/Url.hpp>

#include "transport/TransportFactory.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
  void expect(bool condition, const char *message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  bool contains(const std::string &text, const std::string &needle)
  {
    return text.find(needle) != std::string::npos;
  }

  template <typename Exception, typename Function>
  void expect_throw(Function &&function, const char *message)
  {
    try
    {
      function();
    }
    catch (const Exception &)
    {
      return;
    }

    throw std::runtime_error(message);
  }

  void test_request_exception_message()
  {
    const vix::requests::RequestException error("base error");

    expect(std::string(error.what()) == "base error",
           "RequestException message should match");
  }

  void test_invalid_url_exception()
  {
    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http://example.com:bad"));
        },
        "invalid URL should throw InvalidUrlException");
  }

  void test_transport_exception_inheritance()
  {
    const vix::requests::TransportException transportError("transport error");
    const vix::requests::ConnectionException connectionError("connection error");
    const vix::requests::TimeoutException timeoutError("timeout error");

    const vix::requests::RequestException &transportBase = transportError;
    const vix::requests::TransportException &connectionBase = connectionError;
    const vix::requests::TransportException &timeoutBase = timeoutError;

    expect(std::string(transportBase.what()) == "transport error",
           "TransportException should derive from RequestException");
    expect(std::string(connectionBase.what()) == "connection error",
           "ConnectionException should derive from TransportException");
    expect(std::string(timeoutBase.what()) == "timeout error",
           "TimeoutException should derive from TransportException");
  }

  void test_http_exception_message_and_fields()
  {
    const vix::requests::HttpException error(
        404,
        "Not Found",
        "http://example.com/missing");

    expect(error.status_code() == 404, "status code should be 404");
    expect(error.reason() == "Not Found", "reason should match");
    expect(error.url() == "http://example.com/missing", "URL should match");

    const std::string message = error.what();

    expect(contains(message, "404"), "message should contain status code");
    expect(contains(message, "Not Found"), "message should contain reason");
    expect(contains(message, "http://example.com/missing"),
           "message should contain URL");
  }

  void test_make_http_error_message()
  {
    const std::string full =
        vix::requests::make_http_error_message(
            500,
            "Internal Server Error",
            "http://example.com/api");

    expect(
        full == "HTTP request failed with status 500 (Internal Server Error) for http://example.com/api",
        "full HTTP error message should match");

    const std::string withoutReason =
        vix::requests::make_http_error_message(
            500,
            "",
            "http://example.com/api");

    expect(
        withoutReason == "HTTP request failed with status 500 for http://example.com/api",
        "HTTP error without reason should match");

    const std::string withoutUrl =
        vix::requests::make_http_error_message(
            500,
            "Internal Server Error",
            "");

    expect(
        withoutUrl == "HTTP request failed with status 500 (Internal Server Error)",
        "HTTP error without URL should match");
  }

  void test_response_raise_for_status()
  {
    const vix::requests::Response ok(
        "http://example.com/",
        200,
        "OK");

    ok.raise_for_status();

    const vix::requests::Response notFound(
        "http://example.com/missing",
        404,
        "Not Found");

    expect_throw<vix::requests::HttpException>(
        [&notFound]
        {
          notFound.raise_for_status();
        },
        "404 response should throw HttpException");
  }

  void test_too_many_redirects_exception()
  {
    const vix::requests::TooManyRedirectsException error("too many redirects");

    expect(std::string(error.what()) == "too many redirects",
           "TooManyRedirectsException message should match");
  }

  void test_unsupported_protocol_exception()
  {
    expect_throw<vix::requests::UnsupportedProtocolException>(
        []
        {
          static_cast<void>(
              vix::requests::transport::protocol_from_scheme("ftp"));
        },
        "ftp should throw UnsupportedProtocolException");

    expect_throw<vix::requests::UnsupportedProtocolException>(
        []
        {
          static_cast<void>(
              vix::requests::transport::make_transport(
                  vix::requests::transport::TransportProtocol::Https));
        },
        "HTTPS transport should throw while TLS backend is unavailable");
  }

  void test_scheme_supported()
  {
    expect(
        vix::requests::transport::scheme_supported("http"),
        "http should be supported");

    expect(
        !vix::requests::transport::scheme_supported("https"),
        "https should not be supported yet");

    expect(
        !vix::requests::transport::scheme_supported("ftp"),
        "ftp should not be supported");
  }
}

int main()
{
  try
  {
    test_request_exception_message();
    test_invalid_url_exception();
    test_transport_exception_inheritance();
    test_http_exception_message_and_fields();
    test_make_http_error_message();
    test_response_raise_for_status();
    test_too_many_redirects_exception();
    test_unsupported_protocol_exception();
    test_scheme_supported();

    std::cout << "errors_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "errors_test failed: " << error.what() << '\n';
    return 1;
  }
}

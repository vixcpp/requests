/**
 *
 *  @file response_parser_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests HTTP response parser.
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

#include "http/HttpParser.hpp"

#include <vix/requests/Error.hpp>
#include <vix/requests/Headers.hpp>
#include <vix/requests/Response.hpp>

#include <iostream>
#include <optional>
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

  void test_find_header_end()
  {
    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    const auto end = vix::requests::http::find_header_end(response);

    expect(end.has_value(), "header end should be found");
    expect(*end == response.find("\r\n\r\n") + 4U,
           "header end offset should point after CRLFCRLF");
  }

  void test_parse_response_head()
  {
    const std::string response =
        "HTTP/1.1 201 Created\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "{}";

    const auto head =
        vix::requests::http::parse_response_head(response);

    expect(head.version == "HTTP/1.1", "HTTP version should match");
    expect(head.statusCode == 201, "status code should be 201");
    expect(head.reason == "Created", "reason should be Created");
    expect(head.headers.get("content-type") == "application/json",
           "Content-Type should parse case-insensitively");
    expect(head.headers.get("content-length") == "2",
           "Content-Length should parse");
    expect(head.headerSize == response.find("\r\n\r\n") + 4U,
           "header size should match");
  }

  void test_parse_content_length_response()
  {
    const std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "HelloExtraIgnored";

    const auto response =
        vix::requests::http::parse_response(
            raw,
            "http://example.com/",
            true);

    expect(response.url() == "http://example.com/", "final URL should match");
    expect(response.status_code() == 200, "status code should be 200");
    expect(response.reason() == "OK", "reason should be OK");
    expect(response.header("content-type") == "text/plain",
           "Content-Type should be parsed");
    expect(response.text() == "Hello", "body should respect Content-Length");
    expect(response.size() == 5, "body size should be 5");
    expect(response.ok(), "response should be ok");
  }

  void test_parse_chunked_response()
  {
    const std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "6\r\n"
        " World\r\n"
        "0\r\n"
        "\r\n";

    const auto response =
        vix::requests::http::parse_response(
            raw,
            "http://example.com/",
            true);

    expect(response.status_code() == 200, "status code should be 200");
    expect(response.text() == "Hello World", "chunked body should decode");
  }

  void test_decode_chunked_body_with_extensions()
  {
    const std::string body =
        "5;name=value\r\n"
        "Hello\r\n"
        "6\r\n"
        " World\r\n"
        "0\r\n"
        "\r\n";

    const std::string decoded =
        vix::requests::http::decode_chunked_body(body);

    expect(decoded == "Hello World", "chunked extensions should be ignored");
  }

  void test_connection_close_body()
  {
    const std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello by close";

    const auto response =
        vix::requests::http::parse_response(
            raw,
            "http://example.com/",
            true);

    expect(response.status_code() == 200, "status code should be 200");
    expect(response.text() == "Hello by close",
           "body should be read until connection close");
  }

  void test_no_body_status()
  {
    const std::string raw =
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "IgnoredBody";

    const auto response =
        vix::requests::http::parse_response(
            raw,
            "http://example.com/",
            true);

    expect(response.status_code() == 204, "status code should be 204");
    expect(response.empty(), "204 response body should be empty");
  }

  void test_head_request_no_body()
  {
    const std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "IgnoredBody";

    const auto response =
        vix::requests::http::parse_response(
            raw,
            "http://example.com/",
            false);

    expect(response.status_code() == 200, "status code should be 200");
    expect(response.empty(), "HEAD response body should be empty");
  }

  void test_interim_response_is_skipped()
  {
    const std::string raw =
        "HTTP/1.1 100 Continue\r\n"
        "\r\n"
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    const auto response =
        vix::requests::http::parse_response(
            raw,
            "http://example.com/",
            true);

    expect(response.status_code() == 200, "final status should be 200");
    expect(response.text() == "Hello", "final body should parse");
  }

  void test_detect_body_info()
  {
    vix::requests::Headers headers;
    headers.set("Content-Length", "12");

    const auto info =
        vix::requests::http::detect_body_info(
            200,
            headers,
            true);

    expect(info.framing == vix::requests::http::BodyFraming::ContentLength,
           "body framing should be ContentLength");
    expect(info.contentLength == 12, "content length should be 12");
  }

  void test_transfer_encoding_chunked()
  {
    expect(
        vix::requests::http::transfer_encoding_is_chunked("gzip, chunked"),
        "Transfer-Encoding should detect chunked");

    expect(
        vix::requests::http::transfer_encoding_is_chunked("Chunked"),
        "Transfer-Encoding should be case-insensitive");

    expect(
        !vix::requests::http::transfer_encoding_is_chunked("gzip"),
        "gzip alone should not be chunked");
  }

  void test_invalid_response_errors()
  {
    expect_throw<vix::requests::TransportException>(
        []
        {
          static_cast<void>(
              vix::requests::http::parse_response_head("bad\r\n\r\n"));
        },
        "invalid status line should throw");

    expect_throw<vix::requests::TransportException>(
        []
        {
          static_cast<void>(
              vix::requests::http::parse_response(
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Length: abc\r\n"
                  "\r\n",
                  "http://example.com/",
                  true));
        },
        "invalid Content-Length should throw");

    expect_throw<vix::requests::TransportException>(
        []
        {
          static_cast<void>(
              vix::requests::http::decode_chunked_body(
                  "Z\r\nbad\r\n0\r\n\r\n"));
        },
        "invalid chunk size should throw");
  }
}

int main()
{
  try
  {
    test_find_header_end();
    test_parse_response_head();
    test_parse_content_length_response();
    test_parse_chunked_response();
    test_decode_chunked_body_with_extensions();
    test_connection_close_body();
    test_no_body_status();
    test_head_request_no_body();
    test_interim_response_is_skipped();
    test_detect_body_info();
    test_transfer_encoding_chunked();
    test_invalid_response_errors();

    std::cout << "response_parser_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "response_parser_test failed: " << error.what() << '\n';
    return 1;
  }
}

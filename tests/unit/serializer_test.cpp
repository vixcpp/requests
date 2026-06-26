/**
 *
 *  @file serializer_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests HTTP request serializer.
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

#include "http/HttpSerializer.hpp"

#include <vix/requests/Body.hpp>
#include <vix/requests/Method.hpp>
#include <vix/requests/Request.hpp>
#include <vix/requests/RequestOptions.hpp>

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

  void test_serialize_header_line()
  {
    const std::string line =
        vix::requests::http::serialize_header_line(
            "Accept",
            "application/json");

    expect(line == "Accept: application/json\r\n",
           "header line should serialize with CRLF");
  }

  void test_serialize_headers()
  {
    vix::requests::Headers headers;
    headers.set("Accept", "application/json");
    headers.set("User-Agent", "vix-requests/1.0.0");

    const std::string serialized =
        vix::requests::http::serialize_headers(headers);

    expect(contains(serialized, "Accept: application/json\r\n"),
           "Accept header should serialize");
    expect(contains(serialized, "User-Agent: vix-requests/1.0.0\r\n"),
           "User-Agent header should serialize");
  }

  void test_serialize_get_request_head()
  {
    vix::requests::RequestOptions options;
    options.headers.set("Accept", "application/json");
    options.params.set("page", "1");

    const vix::requests::Request request(
        vix::requests::Method::Get,
        "http://example.com/api/items?sort=new",
        options);

    const std::string head =
        vix::requests::http::serialize_request_head(request);

    expect(
        contains(head, "GET /api/items?sort=new&page=1 HTTP/1.1\r\n"),
        "request line should include method and final target");

    expect(
        contains(head, "Host: example.com\r\n"),
        "Host header should be added");

    expect(
        contains(head, "Accept: application/json\r\n"),
        "Accept header should be preserved");

    expect(
        contains(head, "User-Agent: vix-requests/"),
        "User-Agent header should be added");

    expect(
        contains(head, "Connection: keep-alive\r\n"),
        "Connection header should be added");

    expect(
        !contains(head, "Content-Length:"),
        "GET without body should not include Content-Length");

    expect(
        head.size() >= 4 && head.substr(head.size() - 4) == "\r\n\r\n",
        "request head should end with CRLFCRLF");
  }

  void test_serialize_post_json_request()
  {
    const auto body = vix::requests::json_body(R"({"name":"Vix"})");

    const vix::requests::Request request(
        vix::requests::Method::Post,
        "http://example.com/api/items",
        {},
        body);

    const vix::requests::http::SerializedRequest serialized =
        vix::requests::http::serialize_request(request);

    expect(
        contains(serialized.head, "POST /api/items HTTP/1.1\r\n"),
        "POST request line should serialize");

    expect(
        contains(serialized.head, "Host: example.com\r\n"),
        "Host header should serialize");

    expect(
        contains(serialized.head, "Content-Type: application/json\r\n"),
        "JSON Content-Type should serialize");

    expect(
        contains(serialized.head, "Content-Length: 14\r\n"),
        "Content-Length should match JSON body size");

    expect(
        serialized.body == R"({"name":"Vix"})",
        "serialized body should match JSON body");

    expect(
        serialized.data == serialized.head + serialized.body,
        "serialized data should be head plus body");
  }

  void test_serialize_form_request()
  {
    const auto body = vix::requests::form_body({{"name", "Gaspard"},
                                                {"project", "Vix Requests"}});

    const vix::requests::Request request(
        "POST",
        "http://127.0.0.1:8080/form",
        {},
        body);

    const vix::requests::http::SerializedRequest serialized =
        vix::requests::http::serialize_request(request);

    expect(
        contains(serialized.head, "POST /form HTTP/1.1\r\n"),
        "form request line should serialize");

    expect(
        contains(serialized.head, "Host: 127.0.0.1:8080\r\n"),
        "Host header should include explicit port");

    expect(
        contains(serialized.head,
                 "Content-Type: application/x-www-form-urlencoded\r\n"),
        "form Content-Type should serialize");

    expect(
        serialized.body == "name=Gaspard&project=Vix+Requests",
        "form body should serialize with form encoding");

    expect(
        contains(serialized.head, "Content-Length: 33\r\n"),
        "Content-Length should match form body size");
  }

  void test_basic_auth_header()
  {
    vix::requests::RequestOptions options;
    options.set_basic_auth("gaspard", "secret");

    const vix::requests::Request request(
        vix::requests::Method::Get,
        "http://example.com/private",
        options);

    const std::string head =
        vix::requests::http::serialize_request_head(request);

    expect(
        contains(head, "Authorization: Basic Z2FzcGFyZDpzZWNyZXQ=\r\n"),
        "Basic auth header should serialize");
  }

  void test_custom_headers_are_not_overwritten()
  {
    vix::requests::RequestOptions options;
    options.headers.set("Host", "custom.host");
    options.headers.set("User-Agent", "custom-agent");
    options.headers.set("Connection", "close");

    const vix::requests::Request request(
        vix::requests::Method::Get,
        "http://example.com/",
        options);

    const std::string head =
        vix::requests::http::serialize_request_head(request);

    expect(
        contains(head, "Host: custom.host\r\n"),
        "custom Host should be preserved");

    expect(
        contains(head, "User-Agent: custom-agent\r\n"),
        "custom User-Agent should be preserved");

    expect(
        contains(head, "Connection: close\r\n"),
        "custom Connection should be preserved");

    expect(
        !contains(head, "Host: example.com\r\n"),
        "default Host should not overwrite custom Host");
  }

  void test_head_request_serialization()
  {
    const vix::requests::Request request(
        vix::requests::Method::Head,
        "http://example.com/status");

    const std::string head =
        vix::requests::http::serialize_request_head(request);

    expect(
        contains(head, "HEAD /status HTTP/1.1\r\n"),
        "HEAD request line should serialize");

    expect(
        !contains(head, "Content-Length:"),
        "HEAD without body should not include Content-Length");
  }
}

int main()
{
  try
  {
    test_serialize_header_line();
    test_serialize_headers();
    test_serialize_get_request_head();
    test_serialize_post_json_request();
    test_serialize_form_request();
    test_basic_auth_header();
    test_custom_headers_are_not_overwritten();
    test_head_request_serialization();

    std::cout << "serializer_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "serializer_test failed: " << error.what() << '\n';
    return 1;
  }
}

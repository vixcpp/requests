/**
 *
 *  @file headers_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests headers.
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
#include <vix/requests/Headers.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

  void test_empty_headers()
  {
    const vix::requests::Headers headers;

    expect(headers.empty(), "headers should be empty");
    expect(headers.size() == 0, "header size should be zero");
    expect(!headers.has("Content-Type"), "Content-Type should not exist");
    expect(!headers.get("Content-Type").has_value(), "get should return nullopt");
    expect(headers.get_all("Content-Type").empty(), "get_all should be empty");
  }

  void test_set_and_get_case_insensitive()
  {
    vix::requests::Headers headers;
    headers.set("Content-Type", "application/json");

    expect(!headers.empty(), "headers should not be empty");
    expect(headers.size() == 1, "header size should be one");
    expect(headers.has("content-type"), "lookup should be case-insensitive");
    expect(headers.has("CONTENT-TYPE"), "lookup should be case-insensitive");

    const auto value = headers.get("content-type");
    expect(value.has_value(), "header value should exist");
    expect(*value == "application/json", "header value should match");
  }

  void test_set_preserves_original_name()
  {
    vix::requests::Headers headers;
    headers.set("Content-Type", "application/json");
    headers.set("content-type", "text/plain");

    expect(headers.size() == 1, "set should replace existing header");
    expect(headers.entries().front().name == "Content-Type",
           "set should preserve original header casing");
    expect(headers.entries().front().value == "text/plain",
           "set should replace value");
  }

  void test_append_keeps_duplicates()
  {
    vix::requests::Headers headers;
    headers.append("Set-Cookie", "a=1");
    headers.append("set-cookie", "b=2");

    expect(headers.size() == 2, "append should keep duplicates");

    const std::vector<std::string> values = headers.get_all("SET-cookie");

    expect(values.size() == 2, "get_all should return duplicate values");
    expect(values[0] == "a=1", "first cookie should match");
    expect(values[1] == "b=2", "second cookie should match");
  }

  void test_remove_case_insensitive()
  {
    vix::requests::Headers headers;
    headers.set("Accept", "application/json");
    headers.set("User-Agent", "vix-requests/1.0.0");
    headers.append("accept", "text/plain");

    const std::size_t removed = headers.remove("ACCEPT");

    expect(removed == 2, "remove should delete all matching headers");
    expect(!headers.has("accept"), "accept should be removed");
    expect(headers.size() == 1, "one header should remain");
    expect(headers.has("User-Agent"), "User-Agent should remain");
  }

  void test_trim_optional_whitespace()
  {
    vix::requests::Headers headers;
    headers.set("Accept", "  application/json\t");

    const auto value = headers.get("accept");

    expect(value.has_value(), "trimmed value should exist");
    expect(*value == "application/json", "optional whitespace should be trimmed");
  }

  void test_initializer_list()
  {
    vix::requests::Headers headers{
        {"Accept", "application/json"},
        {"User-Agent", "vix-requests/1.0.0"}};

    expect(headers.size() == 2, "initializer list should add headers");
    expect(headers.get("accept") == "application/json", "Accept should match");
    expect(headers.get("user-agent") == "vix-requests/1.0.0", "User-Agent should match");
  }

  void test_clear()
  {
    vix::requests::Headers headers;
    headers.set("Accept", "application/json");
    headers.set("User-Agent", "vix-requests/1.0.0");

    headers.clear();

    expect(headers.empty(), "headers should be empty after clear");
    expect(headers.size() == 0, "size should be zero after clear");
  }

  void test_invalid_header_names()
  {
    expect_throw<vix::requests::RequestException>(
        []
        {
          vix::requests::Headers headers;
          headers.set("", "value");
        },
        "empty header name should throw");

    expect_throw<vix::requests::RequestException>(
        []
        {
          vix::requests::Headers headers;
          headers.set("Bad Header", "value");
        },
        "header name with space should throw");

    expect_throw<vix::requests::RequestException>(
        []
        {
          vix::requests::Headers headers;
          headers.set("Bad:Header", "value");
        },
        "header name with colon should throw");
  }
}

int main()
{
  try
  {
    test_empty_headers();
    test_set_and_get_case_insensitive();
    test_set_preserves_original_name();
    test_append_keeps_duplicates();
    test_remove_case_insensitive();
    test_trim_optional_whitespace();
    test_initializer_list();
    test_clear();
    test_invalid_header_names();

    std::cout << "headers_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "headers_test failed: " << error.what() << '\n';
    return 1;
  }
}

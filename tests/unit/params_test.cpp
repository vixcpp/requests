/**
 *
 *  @file params_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests query params.
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

#include <vix/requests/Params.hpp>

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

  void test_empty_params()
  {
    const vix::requests::Params params;

    expect(params.empty(), "params should be empty");
    expect(params.size() == 0, "params size should be zero");
    expect(!params.has("page"), "page should not exist");
    expect(!params.get("page").has_value(), "get should return nullopt");
    expect(params.get_all("page").empty(), "get_all should be empty");
    expect(params.to_query_string().empty(), "query string should be empty");
  }

  void test_set_and_get()
  {
    vix::requests::Params params;
    params.set("page", "1");
    params.set("q", "vix requests");

    expect(!params.empty(), "params should not be empty");
    expect(params.size() == 2, "params size should be two");
    expect(params.has("page"), "page should exist");
    expect(params.get("page") == "1", "page should be 1");
    expect(params.get("q") == "vix requests", "q should match");
  }

  void test_set_replaces_and_removes_duplicates()
  {
    vix::requests::Params params;
    params.append("tag", "cpp");
    params.append("tag", "vix");
    params.append("tag", "requests");

    expect(params.size() == 3, "append should keep duplicates");

    params.set("tag", "final");

    expect(params.size() == 1, "set should remove duplicate keys");
    expect(params.get("tag") == "final", "tag should be replaced");
  }

  void test_append_keeps_duplicates()
  {
    vix::requests::Params params;
    params.append("tag", "cpp");
    params.append("tag", "http");

    const std::vector<std::string> values = params.get_all("tag");

    expect(params.size() == 2, "append should keep duplicate keys");
    expect(values.size() == 2, "get_all should return all values");
    expect(values[0] == "cpp", "first value should match");
    expect(values[1] == "http", "second value should match");
  }

  void test_remove()
  {
    vix::requests::Params params;
    params.append("tag", "cpp");
    params.append("tag", "http");
    params.set("page", "1");

    const std::size_t removed = params.remove("tag");

    expect(removed == 2, "remove should delete both tag params");
    expect(!params.has("tag"), "tag should be removed");
    expect(params.size() == 1, "one param should remain");
    expect(params.has("page"), "page should remain");
  }

  void test_clear()
  {
    vix::requests::Params params;
    params.set("page", "1");
    params.set("q", "vix");

    params.clear();

    expect(params.empty(), "params should be empty after clear");
    expect(params.size() == 0, "size should be zero after clear");
  }

  void test_query_string_encoding()
  {
    vix::requests::Params params;
    params.set("page", "1");
    params.set("q", "hello world");
    params.set("symbol", "a+b&c=d");

    const std::string query = params.to_query_string();

    expect(
        query == "page=1&q=hello%20world&symbol=a%2Bb%26c%3Dd",
        "query string should be percent encoded");
  }

  void test_initializer_list()
  {
    vix::requests::Params params{
        {"page", "1"},
        {"q", "vix requests"}};

    expect(params.size() == 2, "initializer list should add params");
    expect(params.get("page") == "1", "page should match");
    expect(params.get("q") == "vix requests", "q should match");
  }

  void test_url_encode_decode()
  {
    expect(
        vix::requests::url_encode("hello world") == "hello%20world",
        "space should encode as %20");

    expect(
        vix::requests::url_encode("a+b&c=d") == "a%2Bb%26c%3Dd",
        "reserved chars should be encoded");

    expect(
        vix::requests::url_decode("hello%20world") == "hello world",
        "%20 should decode as space");

    expect(
        vix::requests::url_decode("a%2Bb%26c%3Dd") == "a+b&c=d",
        "reserved chars should decode");

    expect(
        vix::requests::url_decode("bad%ZZvalue") == "bad%ZZvalue",
        "invalid percent sequence should be preserved");
  }

  void test_form_url_encode_decode()
  {
    expect(
        vix::requests::form_url_encode("hello world") == "hello+world",
        "form space should encode as +");

    expect(
        vix::requests::form_url_encode("a+b&c=d") == "a%2Bb%26c%3Dd",
        "reserved chars should be encoded");

    expect(
        vix::requests::form_url_decode("hello+world") == "hello world",
        "form + should decode as space");

    expect(
        vix::requests::form_url_decode("a%2Bb%26c%3Dd") == "a+b&c=d",
        "reserved chars should decode");
  }
}

int main()
{
  try
  {
    test_empty_params();
    test_set_and_get();
    test_set_replaces_and_removes_duplicates();
    test_append_keeps_duplicates();
    test_remove();
    test_clear();
    test_query_string_encoding();
    test_initializer_list();
    test_url_encode_decode();
    test_form_url_encode_decode();

    std::cout << "params_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "params_test failed: " << error.what() << '\n';
    return 1;
  }
}

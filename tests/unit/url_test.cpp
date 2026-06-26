/**
 *
 *  @file url_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests URL parsing.
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
#include <vix/requests/Params.hpp>
#include <vix/requests/Url.hpp>

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

  void test_parse_basic_http_url()
  {
    const auto url = vix::requests::Url::parse("http://example.com");

    expect(url.scheme() == "http", "scheme should be http");
    expect(url.host() == "example.com", "host should be example.com");
    expect(url.port() == 80, "default http port should be 80");
    expect(!url.has_explicit_port(), "port should not be explicit");
    expect(url.path() == "/", "default path should be /");
    expect(url.query().empty(), "query should be empty");
    expect(url.fragment().empty(), "fragment should be empty");
    expect(url.authority() == "example.com", "authority should be host only");
    expect(url.origin() == "http://example.com", "origin should be valid");
    expect(url.request_target() == "/", "request target should be /");
    expect(url.without_fragment() == "http://example.com/", "url without fragment should include /");
  }

  void test_parse_full_url()
  {
    const auto url = vix::requests::Url::parse(
        "http://localhost:8080/api/items?page=1#top");

    expect(url.scheme() == "http", "scheme should be http");
    expect(url.host() == "localhost", "host should be localhost");
    expect(url.port() == 8080, "port should be 8080");
    expect(url.explicit_port().has_value(), "explicit port should exist");
    expect(*url.explicit_port() == 8080, "explicit port should be 8080");
    expect(url.path() == "/api/items", "path should be /api/items");
    expect(url.query() == "page=1", "query should be page=1");
    expect(url.fragment() == "top", "fragment should be top");
    expect(url.authority() == "localhost:8080", "authority should include port");
    expect(url.origin() == "http://localhost:8080", "origin should include port");
    expect(url.request_target() == "/api/items?page=1", "request target should include query");
    expect(url.without_fragment() == "http://localhost:8080/api/items?page=1",
           "without_fragment should drop fragment");
    expect(url.to_string() == "http://localhost:8080/api/items?page=1#top",
           "to_string should keep fragment");
  }

  void test_https_url_is_parsed_but_transport_can_reject_later()
  {
    const auto url = vix::requests::Url::parse("https://example.com/path");

    expect(url.scheme() == "https", "scheme should be https");
    expect(url.is_https(), "url should be https");
    expect(url.port() == 443, "default https port should be 443");
    expect(url.request_target() == "/path", "request target should be /path");
  }

  void test_ipv6_url()
  {
    const auto url = vix::requests::Url::parse("http://[::1]:8080/status");

    expect(url.scheme() == "http", "scheme should be http");
    expect(url.host() == "::1", "host should be ::1");
    expect(url.port() == 8080, "port should be 8080");
    expect(url.authority() == "[::1]:8080", "authority should wrap IPv6 host");
    expect(url.request_target() == "/status", "request target should be /status");
  }

  void test_with_params()
  {
    vix::requests::Params params;
    params.set("page", "1");
    params.set("q", "hello world");

    const auto url = vix::requests::Url::parse(
        "http://example.com/search?sort=new");

    const auto finalUrl = url.with_params(params);

    expect(finalUrl.query() == "sort=new&page=1&q=hello%20world",
           "params should be appended and encoded");
    expect(finalUrl.request_target() == "/search?sort=new&page=1&q=hello%20world",
           "request target should include appended params");
  }

  void test_append_query_string()
  {
    expect(
        vix::requests::append_query_string(
            "http://example.com/path",
            "a=1") == "http://example.com/path?a=1",
        "append_query_string should add ?");

    expect(
        vix::requests::append_query_string(
            "http://example.com/path?x=1",
            "a=1") == "http://example.com/path?x=1&a=1",
        "append_query_string should add &");

    expect(
        vix::requests::append_query_string(
            "http://example.com/path#top",
            "a=1") == "http://example.com/path?a=1#top",
        "append_query_string should preserve fragment");

    expect(
        vix::requests::append_query_string(
            "http://example.com/path",
            "") == "http://example.com/path",
        "empty query should return original URL");
  }

  void test_invalid_urls()
  {
    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse(""));
        },
        "empty URL should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("example.com"));
        },
        "missing scheme should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http:/example.com"));
        },
        "missing authority should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http://"));
        },
        "empty authority should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http://example.com:abc"));
        },
        "non numeric port should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http://example.com:70000"));
        },
        "out of range port should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http://user@example.com"));
        },
        "userinfo should throw");

    expect_throw<vix::requests::InvalidUrlException>(
        []
        {
          static_cast<void>(vix::requests::Url::parse("http://exa mple.com"));
        },
        "whitespace should throw");
  }
}

int main()
{
  try
  {
    test_parse_basic_http_url();
    test_parse_full_url();
    test_https_url_is_parsed_but_transport_can_reject_later();
    test_ipv6_url();
    test_with_params();
    test_append_query_string();
    test_invalid_urls();

    std::cout << "url_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "url_test failed: " << error.what() << '\n';
    return 1;
  }
}

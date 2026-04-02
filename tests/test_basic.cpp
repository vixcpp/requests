/**
 * @file test_basic.cpp
 * @brief Basic tests for vix::requests
 *
 * @version 0.1.0
 * @author Gaspard Kirira
 * @copyright (c) 2026
 * @license MIT
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <vix/requests/requests.hpp>

namespace
{
  void expect_true(bool condition, const std::string &message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void expect_equal(const std::string &actual,
                    const std::string &expected,
                    const std::string &message)
  {
    if (actual != expected)
    {
      throw std::runtime_error(
          message + "\nExpected: [" + expected + "]\nActual:   [" + actual + "]");
    }
  }

  void expect_equal(int actual,
                    int expected,
                    const std::string &message)
  {
    if (actual != expected)
    {
      throw std::runtime_error(
          message + "\nExpected: [" + std::to_string(expected) +
          "]\nActual:   [" + std::to_string(actual) + "]");
    }
  }

  void test_url_encode()
  {
    expect_equal(vix::requests::url_encode("hello world"),
                 "hello%20world",
                 "url_encode should encode spaces");

    expect_equal(vix::requests::url_encode("name=John&age=20"),
                 "name%3DJohn%26age%3D20",
                 "url_encode should encode reserved characters");
  }

  void test_build_query_string()
  {
    vix::requests::Params params{
        {"q", "hello world"},
        {"lang", "en"}};

    const std::string query = vix::requests::build_query_string(params);

    expect_true(!query.empty(), "build_query_string should not return an empty string");
    expect_true(query.find("q=hello%20world") != std::string::npos,
                "build_query_string should encode query parameter values");
    expect_true(query.find("lang=en") != std::string::npos,
                "build_query_string should include all parameters");
  }

  void test_with_params()
  {
    vix::requests::Params params{
        {"page", "1"},
        {"limit", "10"}};

    const std::string url = vix::requests::with_params("https://example.com/users", params);

    expect_true(url.find("https://example.com/users?") == 0,
                "with_params should append a query string when URL has no existing query");
    expect_true(url.find("page=1") != std::string::npos,
                "with_params should include page parameter");
    expect_true(url.find("limit=10") != std::string::npos,
                "with_params should include limit parameter");
  }

  void test_with_params_existing_query()
  {
    vix::requests::Params params{
        {"page", "2"}};

    const std::string url =
        vix::requests::with_params("https://example.com/users?active=true", params);

    expect_true(url.find('&') != std::string::npos,
                "with_params should append with '&' when query string already exists");
    expect_true(url.find("active=true") != std::string::npos,
                "with_params should preserve existing query string");
    expect_true(url.find("page=2") != std::string::npos,
                "with_params should append new parameters");
  }

  void test_response_ok_and_bytes()
  {
    vix::requests::Response response;
    response.status_code = 200;
    response.text = "hello";

    expect_true(response.ok(), "Response::ok should return true for 2xx responses");

    const auto bytes = response.bytes();
    expect_equal(static_cast<int>(bytes.size()), 5,
                 "Response::bytes should return all response body bytes");
    expect_true(bytes[0] == static_cast<unsigned char>('h'),
                "Response::bytes should preserve response content");
  }

  void test_response_header_lookup()
  {
    vix::requests::Response response;
    response.headers["Content-Type"] = "application/json";
    response.headers["X-Test"] = "ok";

    const auto content_type = response.header("content-type");
    const auto x_test = response.header("X-Test");
    const auto missing = response.header("missing");

    expect_true(content_type.has_value(),
                "Response::header should find headers case-insensitively");
    expect_equal(content_type.value(), "application/json",
                 "Response::header should return the correct header value");

    expect_true(x_test.has_value(),
                "Response::header should find existing headers");
    expect_equal(x_test.value(), "ok",
                 "Response::header should return the correct header value");

    expect_true(!missing.has_value(),
                "Response::header should return std::nullopt for missing headers");
  }

  void test_response_raise_for_status_success()
  {
    vix::requests::Response response;
    response.status_code = 201;
    response.reason = "Created";

    response.raise_for_status();
  }

  void test_response_raise_for_status_failure()
  {
    vix::requests::Response response;
    response.status_code = 404;
    response.reason = "Not Found";

    bool thrown = false;

    try
    {
      response.raise_for_status();
    }
    catch (const vix::requests::HttpException &)
    {
      thrown = true;
    }

    expect_true(thrown,
                "Response::raise_for_status should throw for non-2xx responses");
  }

  void test_session_defaults()
  {
    vix::requests::Session session;

    session.set_header("Accept", "application/json");
    session.defaults().timeout_seconds = 15;
    session.defaults().user_agent = "requests-test/0.1.0";

    expect_equal(session.defaults().headers["Accept"], "application/json",
                 "Session::set_header should store default headers");
    expect_equal(static_cast<int>(session.defaults().timeout_seconds), 15,
                 "Session defaults should store timeout");
    expect_equal(session.defaults().user_agent, "requests-test/0.1.0",
                 "Session defaults should store user-agent");
  }

  void test_request_options_defaults()
  {
    vix::requests::RequestOptions options;

    expect_true(options.headers.empty(),
                "RequestOptions headers should be empty by default");
    expect_true(options.params.empty(),
                "RequestOptions params should be empty by default");
    expect_equal(options.body, "",
                 "RequestOptions body should be empty by default");
    expect_equal(options.json, "",
                 "RequestOptions json should be empty by default");
    expect_equal(options.username, "",
                 "RequestOptions username should be empty by default");
    expect_equal(options.password, "",
                 "RequestOptions password should be empty by default");
    expect_equal(static_cast<int>(options.timeout_seconds), 0,
                 "RequestOptions timeout should be 0 by default");
    expect_true(options.follow_redirects,
                "RequestOptions should follow redirects by default");
    expect_true(options.verify_tls,
                "RequestOptions should verify TLS by default");
    expect_true(options.compressed,
                "RequestOptions should enable compression by default");
  }

  void run_all_tests()
  {
    test_url_encode();
    test_build_query_string();
    test_with_params();
    test_with_params_existing_query();
    test_response_ok_and_bytes();
    test_response_header_lookup();
    test_response_raise_for_status_success();
    test_response_raise_for_status_failure();
    test_session_defaults();
    test_request_options_defaults();
  }
} // namespace

int main()
{
  try
  {
    run_all_tests();
    std::cout << "All requests tests passed.\n";
    return EXIT_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Test failure: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
}

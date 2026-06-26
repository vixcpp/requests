/**
 *
 *  @file http_client_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Integration tests for Vix Requests HTTP client.
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
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>

#include <exception>
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

  void test_simple_get()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.method == "GET" && request.path == "/hello")
          {
            return vix::requests::tests::local_http_response(
                200,
                "Hello Vix");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    const auto response = vix::requests::get(server.url("/hello"));

    expect(response.status_code() == 200, "GET status should be 200");
    expect(response.ok(), "GET response should be ok");
    expect(response.text() == "Hello Vix", "GET body should match");
    expect(response.url() == server.url("/hello"), "final URL should match");
  }

  void test_get_with_params_and_headers()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.method != "GET")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong method");
          }

          if (request.path != "/search")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong path");
          }

          if (!contains(request.query, "page=1") ||
              !contains(request.query, "q=vix%20requests"))
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong query");
          }

          const auto it = request.headers.find("accept");

          if (it == request.headers.end() ||
              it->second != "application/json")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong accept");
          }

          return vix::requests::tests::local_http_response(
              200,
              R"({"ok":true})",
              "application/json");
        });

    vix::requests::RequestOptions options;
    options.headers.set("Accept", "application/json");
    options.params.set("page", "1");
    options.params.set("q", "vix requests");

    const auto response =
        vix::requests::get(server.url("/search"), options);

    expect(response.status_code() == 200, "GET with params should succeed");
    expect(response.content_type() == "application/json",
           "Content-Type should be application/json");
    expect(response.text() == R"({"ok":true})", "JSON response should match");
  }


  vix::async::core::task<void> run_async_get(
      vix::async::core::io_context &ctx,
      std::string url,
      vix::requests::Response &response,
      std::exception_ptr &error)
  {
    try
    {
      auto requestTask = vix::requests::async_get(ctx, url);
      response = co_await requestTask;
    }
    catch (...)
    {
      error = std::current_exception();
    }

    ctx.stop();
    co_return;
  }

  void test_async_get()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.method == "GET" && request.path == "/async")
          {
            return vix::requests::tests::local_http_response(
                200,
                "async ok");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    vix::async::core::io_context ctx;
    vix::requests::Response response;
    std::exception_ptr error;

    auto app = run_async_get(ctx, server.url("/async"), response, error);
    ctx.post(app.handle());
    ctx.run();

    if (error)
    {
      std::rethrow_exception(error);
    }

    expect(response.status_code() == 200, "async GET status should be 200");
    expect(response.text() == "async ok", "async GET body should match");
  }

  void test_post_json()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.method != "POST" || request.path != "/items")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong route");
          }

          const auto contentType = request.headers.find("content-type");

          if (contentType == request.headers.end() ||
              contentType->second != "application/json")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong content type");
          }

          if (request.body != R"({"name":"Vix"})")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong body");
          }

          return vix::requests::tests::local_http_response(
              201,
              "created");
        });

    const auto response =
        vix::requests::post(
            server.url("/items"),
            vix::requests::json_body(R"({"name":"Vix"})"));

    expect(response.status_code() == 201, "POST JSON should return 201");
    expect(response.text() == "created", "POST JSON body should match");
  }

  void test_post_form()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.method != "POST" || request.path != "/form")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong route");
          }

          if (request.body != "name=Gaspard&project=Vix+Requests")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong form body");
          }

          return vix::requests::tests::local_http_response(
              200,
              "form ok");
        });

    const auto response =
        vix::requests::post(
            server.url("/form"),
            vix::requests::form_body({{"name", "Gaspard"},
                                      {"project", "Vix Requests"}}));

    expect(response.status_code() == 200, "POST form should return 200");
    expect(response.text() == "form ok", "POST form response should match");
  }

  void test_head_request()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.method != "HEAD" || request.path != "/status")
          {
            return vix::requests::tests::local_http_response(
                500,
                "wrong route");
          }

          return vix::requests::tests::local_http_response(
              200,
              "ignored body");
        });

    const auto response = vix::requests::head(server.url("/status"));

    expect(response.status_code() == 200, "HEAD should return 200");
    expect(response.empty(), "HEAD response body should be empty");
  }

  void test_http_error_status()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &)
        {
          return vix::requests::tests::local_http_response(
              404,
              "missing");
        });

    const auto response = vix::requests::get(server.url("/missing"));

    expect(response.status_code() == 404, "status should be 404");
    expect(!response.ok(), "404 should not be ok");
    expect(response.is_error(), "404 should be error");

    bool thrown = false;

    try
    {
      response.raise_for_status();
    }
    catch (const vix::requests::HttpException &)
    {
      thrown = true;
    }

    expect(thrown, "raise_for_status should throw on 404");
  }

  void test_unsupported_https()
  {
    bool thrown = false;

    try
    {
      static_cast<void>(vix::requests::get("https://example.com/"));
    }
    catch (const vix::requests::UnsupportedProtocolException &)
    {
      thrown = true;
    }

    expect(thrown, "HTTPS should throw while TLS transport is unavailable");
  }
}

int main()
{
  try
  {
    test_simple_get();
    test_get_with_params_and_headers();
    test_async_get();
    test_post_json();
    test_post_form();
    test_head_request();
    test_http_error_status();
    test_unsupported_https();

    std::cout << "http_client_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "http_client_test failed: " << error.what() << '\n';
    return 1;
  }
}

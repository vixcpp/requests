/**
 *
 *  @file redirect_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Integration tests for Vix Requests redirects.
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

  void test_follow_absolute_redirect()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.path == "/start")
          {
            return vix::requests::tests::local_http_redirect("/final");
          }

          if (request.path == "/final")
          {
            return vix::requests::tests::local_http_response(
                200,
                "redirect ok");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    const auto response = vix::requests::get(server.url("/start"));

    expect(response.status_code() == 200, "redirect final status should be 200");
    expect(response.text() == "redirect ok", "redirect final body should match");
    expect(response.url() == server.url("/final"), "final URL should be /final");
  }

  void test_disable_redirects()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.path == "/start")
          {
            return vix::requests::tests::local_http_redirect("/final");
          }

          return vix::requests::tests::local_http_response(
              200,
              "should not follow");
        });

    vix::requests::RequestOptions options;
    options.follow_redirects = false;

    const auto response =
        vix::requests::get(server.url("/start"), options);

    expect(response.status_code() == 302, "redirect should not be followed");
    expect(response.location() == "/final", "Location header should remain");
  }

  void test_redirect_loop_detection()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.path == "/a")
          {
            return vix::requests::tests::local_http_redirect("/b");
          }

          if (request.path == "/b")
          {
            return vix::requests::tests::local_http_redirect("/a");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    bool thrown = false;

    try
    {
      static_cast<void>(vix::requests::get(server.url("/a")));
    }
    catch (const vix::requests::TooManyRedirectsException &)
    {
      thrown = true;
    }

    expect(thrown, "redirect loop should throw");
  }

  void test_max_redirects()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.path == "/start")
          {
            return vix::requests::tests::local_http_redirect("/step1");
          }

          if (request.path == "/step1")
          {
            return vix::requests::tests::local_http_redirect("/step2");
          }

          if (request.path == "/step2")
          {
            return vix::requests::tests::local_http_response(
                200,
                "too late");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    vix::requests::RequestOptions options;
    options.max_redirects = 1;

    bool thrown = false;

    try
    {
      static_cast<void>(vix::requests::get(server.url("/start"), options));
    }
    catch (const vix::requests::TooManyRedirectsException &)
    {
      thrown = true;
    }

    expect(thrown, "max_redirects should throw");
  }

  void test_post_302_rewrites_to_get()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.path == "/submit")
          {
            return vix::requests::tests::local_http_redirect("/result");
          }

          if (request.path == "/result")
          {
            if (request.method != "GET")
            {
              return vix::requests::tests::local_http_response(
                  500,
                  "method was not rewritten");
            }

            if (!request.body.empty())
            {
              return vix::requests::tests::local_http_response(
                  500,
                  "body should not be preserved");
            }

            return vix::requests::tests::local_http_response(
                200,
                "rewritten");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    const auto response =
        vix::requests::post(
            server.url("/submit"),
            vix::requests::json_body(R"({"name":"Vix"})"));

    expect(response.status_code() == 200, "POST 302 should end with 200");
    expect(response.text() == "rewritten", "POST 302 should rewrite to GET");
  }

  void test_get_307_preserves_method()
  {
    vix::requests::tests::LocalHttpServer server(
        [](const vix::requests::tests::LocalHttpRequest &request)
        {
          if (request.path == "/old")
          {
            return vix::requests::tests::local_http_response(
                307,
                {},
                {},
                "Location: /new\r\n");
          }

          if (request.path == "/new")
          {
            if (request.method != "GET")
            {
              return vix::requests::tests::local_http_response(
                  500,
                  "wrong method");
            }

            return vix::requests::tests::local_http_response(
                200,
                "preserved");
          }

          return vix::requests::tests::local_http_response(
              404,
              "not found");
        });

    const auto response = vix::requests::get(server.url("/old"));

    expect(response.status_code() == 200, "307 redirect should end with 200");
    expect(response.text() == "preserved", "307 should preserve GET method");
  }
}

int main()
{
  try
  {
    test_follow_absolute_redirect();
    test_disable_redirects();
    test_redirect_loop_detection();
    test_max_redirects();
    test_post_302_rewrites_to_get();
    test_get_307_preserves_method();

    std::cout << "redirect_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "redirect_test failed: " << error.what() << '\n';
    return 1;
  }
}

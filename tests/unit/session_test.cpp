/**
 *
 *  @file session_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests session defaults.
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

#include <vix/requests/Session.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
  void expect(bool condition, const char *message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_default_session_is_empty_but_valid()
  {
    vix::requests::Session session;

    expect(session.headers().empty(), "default headers should be empty");
    expect(session.params().empty(), "default params should be empty");
    expect(!session.defaults().auth.configured(), "default auth should be empty");
    expect(session.defaults().follow_redirects, "redirects should be enabled by default");
    expect(session.defaults().max_redirects == 10, "default max redirects should be 10");
    expect(session.defaults().verify_tls, "TLS verification should be enabled by default");
    expect(session.defaults().keep_alive, "keep alive should be enabled by default");
    expect(session.defaults().user_agent == "vix-requests/1.0.0",
           "default User-Agent should be vix-requests/1.0.0");
  }

  void test_session_headers()
  {
    vix::requests::Session session;

    session.set_header("Accept", "application/json");
    session.set_header("User-Agent", "custom-agent");

    expect(session.headers().size() == 2, "session should have two headers");
    expect(session.headers().get("accept") == "application/json",
           "Accept header should be stored");
    expect(session.headers().get("user-agent") == "custom-agent",
           "User-Agent header should be stored");

    session.remove_header("ACCEPT");

    expect(!session.headers().has("Accept"), "Accept header should be removed");
    expect(session.headers().has("User-Agent"), "User-Agent header should remain");
  }

  void test_session_params()
  {
    vix::requests::Session session;

    session.set_param("token", "abc");
    session.set_param("page", "1");

    expect(session.params().size() == 2, "session should have two params");
    expect(session.params().get("token") == "abc", "token param should be stored");
    expect(session.params().get("page") == "1", "page param should be stored");

    session.remove_param("page");

    expect(!session.params().has("page"), "page param should be removed");
    expect(session.params().has("token"), "token param should remain");
  }

  void test_session_timeout()
  {
    vix::requests::Session session;

    session.timeout() = std::chrono::seconds(5);

    expect(session.timeout().active(), "timeout should be active");
    expect(session.timeout().connect() == std::chrono::seconds(5),
           "connect timeout should be 5 seconds");
    expect(session.timeout().read() == std::chrono::seconds(5),
           "read timeout should be 5 seconds");
    expect(session.timeout().total() == std::chrono::seconds(5),
           "total timeout should be 5 seconds");
  }

  void test_session_basic_auth()
  {
    vix::requests::Session session;

    session.set_basic_auth("gaspard", "secret");

    expect(session.defaults().auth.configured(), "auth should be configured");
    expect(session.defaults().auth.username == "gaspard", "username should match");
    expect(session.defaults().auth.password == "secret", "password should match");
  }

  void test_set_defaults()
  {
    vix::requests::RequestOptions options;
    options.headers.set("Accept", "application/json");
    options.params.set("token", "abc");
    options.set_basic_auth("user", "pass");
    options.set_timeout(std::chrono::seconds(3));
    options.follow_redirects = false;
    options.keep_alive = false;
    options.user_agent = "custom-agent";

    vix::requests::Session session;
    session.set_defaults(options);

    expect(session.headers().get("Accept") == "application/json",
           "default header should be replaced");
    expect(session.params().get("token") == "abc",
           "default param should be replaced");
    expect(session.defaults().auth.username == "user",
           "default auth username should match");
    expect(session.defaults().auth.password == "pass",
           "default auth password should match");
    expect(session.timeout().connect() == std::chrono::seconds(3),
           "default timeout should match");
    expect(!session.defaults().follow_redirects,
           "follow_redirects should be replaced");
    expect(!session.defaults().keep_alive,
           "keep_alive should be replaced");
    expect(session.defaults().user_agent == "custom-agent",
           "User-Agent should be replaced");
  }

  void test_merge_request_options()
  {
    vix::requests::RequestOptions base;
    base.headers.set("Accept", "application/json");
    base.params.set("token", "abc");
    base.set_basic_auth("base-user", "base-pass");
    base.set_timeout(std::chrono::seconds(5));
    base.user_agent = "base-agent";

    vix::requests::RequestOptions overrideOptions;
    overrideOptions.headers.set("X-Test", "yes");
    overrideOptions.params.set("page", "1");
    overrideOptions.set_basic_auth("override-user", "override-pass");
    overrideOptions.set_timeout(std::chrono::seconds(2));
    overrideOptions.user_agent = "override-agent";

    const auto merged =
        vix::requests::merge_request_options(base, overrideOptions);

    expect(merged.headers.get("Accept") == "application/json",
           "base header should remain");
    expect(merged.headers.get("X-Test") == "yes",
           "override header should be added");
    expect(merged.params.get("token") == "abc",
           "base param should remain");
    expect(merged.params.get("page") == "1",
           "override param should be added");
    expect(merged.auth.username == "override-user",
           "override auth username should win");
    expect(merged.auth.password == "override-pass",
           "override auth password should win");
    expect(merged.timeout.connect() == std::chrono::seconds(2),
           "override timeout should win");
    expect(merged.user_agent == "override-agent",
           "override User-Agent should win");
  }

  void test_move_session()
  {
    vix::requests::Session session;
    session.set_header("Accept", "application/json");
    session.set_param("token", "abc");

    vix::requests::Session moved(std::move(session));

    expect(moved.headers().get("Accept") == "application/json",
           "moved session should keep headers");
    expect(moved.params().get("token") == "abc",
           "moved session should keep params");

    vix::requests::Session assigned;
    assigned = std::move(moved);

    expect(assigned.headers().get("Accept") == "application/json",
           "move-assigned session should keep headers");
    expect(assigned.params().get("token") == "abc",
           "move-assigned session should keep params");
  }

  void test_clear_cookies_is_safe()
  {
    vix::requests::Session session;

    session.clear_cookies();

    expect(true, "clear_cookies should be safe on an empty jar");
  }
}

int main()
{
  try
  {
    test_default_session_is_empty_but_valid();
    test_session_headers();
    test_session_params();
    test_session_timeout();
    test_session_basic_auth();
    test_set_defaults();
    test_merge_request_options();
    test_move_session();
    test_clear_cookies_is_safe();

    std::cout << "session_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "session_test failed: " << error.what() << '\n';
    return 1;
  }
}

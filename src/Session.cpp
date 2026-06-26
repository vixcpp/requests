/**
 *
 *  @file Session.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Reusable HTTP session implementation.
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
#include <vix/requests/Error.hpp>
#include <vix/async/core/io_context.hpp>

#include "http/CookieJar.hpp"
#include "http/RedirectPolicy.hpp"
#include "transport/TransportFactory.hpp"

#include <memory>
#include <utility>

namespace vix::requests
{
  /**
   * @brief Internal session runtime.
   */
  class SessionRuntime
  {
  public:
    /**
     * @brief Default request options.
     */
    RequestOptions defaults;

    /**
     * @brief Session cookie jar.
     */
    http::CookieJar cookies;
  };

  namespace
  {
    namespace core = vix::async::core;

    [[nodiscard]] core::task<Response> send_once_with_cookies_async(
        core::io_context &ctx,
        Request request,
        http::CookieJar &cookies)
    {
      Headers headers = request.effective_headers();

      cookies.remove_expired();
      cookies.apply_to(request.final_url(), headers);

      request.options().headers = std::move(headers);

      auto transport =
          transport::make_transport_for_url(request.final_url());

      Response response = co_await transport->async_send(ctx, request);

      cookies.store_from_response(
          request.final_url(),
          response.headers());

      co_return response;
    }

    core::task<void> drive_sync(
        core::io_context &ctx,
        core::task<Response> &task,
        Response &response,
        std::exception_ptr &error)
    {
      try
      {
        response = co_await task;
      }
      catch (...)
      {
        error = std::current_exception();
      }

      ctx.stop();
      co_return;
    }

    [[nodiscard]] Response run_sync(
        core::io_context &ctx,
        core::task<Response> task)
    {
      Response response;
      std::exception_ptr error;

      auto runner = drive_sync(ctx, task, response, error);

      ctx.post(runner.handle());
      ctx.run();

      if (error)
      {
        std::rethrow_exception(error);
      }

      return response;
    }
  } // namespace

  Session::Session()
      : runtime_(std::make_unique<SessionRuntime>())
  {
  }

  Session::Session(RequestOptions defaults)
      : runtime_(std::make_unique<SessionRuntime>())
  {
    runtime_->defaults = std::move(defaults);
  }

  Session::~Session() = default;

  Session::Session(Session &&other) noexcept = default;

  Session &Session::operator=(Session &&other) noexcept = default;

  RequestOptions &Session::defaults() noexcept
  {
    return runtime_->defaults;
  }

  const RequestOptions &Session::defaults() const noexcept
  {
    return runtime_->defaults;
  }

  void Session::set_defaults(RequestOptions options)
  {
    runtime_->defaults = std::move(options);
  }

  Headers &Session::headers() noexcept
  {
    return runtime_->defaults.headers;
  }

  const Headers &Session::headers() const noexcept
  {
    return runtime_->defaults.headers;
  }

  Params &Session::params() noexcept
  {
    return runtime_->defaults.params;
  }

  const Params &Session::params() const noexcept
  {
    return runtime_->defaults.params;
  }

  Timeout &Session::timeout() noexcept
  {
    return runtime_->defaults.timeout;
  }

  const Timeout &Session::timeout() const noexcept
  {
    return runtime_->defaults.timeout;
  }

  Session &Session::set_header(
      std::string_view name,
      std::string_view value)
  {
    runtime_->defaults.headers.set(name, value);
    return *this;
  }

  Session &Session::remove_header(std::string_view name)
  {
    runtime_->defaults.headers.remove(name);
    return *this;
  }

  Session &Session::set_param(
      std::string_view name,
      std::string_view value)
  {
    runtime_->defaults.params.set(name, value);
    return *this;
  }

  Session &Session::remove_param(std::string_view name)
  {
    runtime_->defaults.params.remove(name);
    return *this;
  }

  Session &Session::set_basic_auth(
      std::string username,
      std::string password)
  {
    runtime_->defaults.set_basic_auth(
        std::move(username),
        std::move(password));

    return *this;
  }

  Session &Session::clear_cookies()
  {
    runtime_->cookies.clear();
    return *this;
  }

  Response Session::send(const Request &request)
  {
    core::io_context ctx;
    return run_sync(ctx, async_send(ctx, request));
  }

  core::task<Response> Session::async_send(
      core::io_context &ctx,
      const Request &request)
  {
    RequestOptions mergedOptions = merge_request_options(
        runtime_->defaults,
        request.options());

    Request current(
        request.method(),
        request.url().without_fragment(),
        std::move(mergedOptions),
        request.body());

    http::RedirectHistory history;

    while (true)
    {
      const std::string currentUrl =
          current.final_url().without_fragment();

      if (history.contains(currentUrl))
      {
        throw TooManyRedirectsException("redirect loop detected");
      }

      history.add(currentUrl);

      /*
       * The Cookie header must be rebuilt for every redirected URL.
       */
      current.options().headers.remove("Cookie");

      auto responseTask = send_once_with_cookies_async(
          ctx,
          current,
          runtime_->cookies);
      Response response = co_await responseTask;

      const http::RedirectDecision decision =
          http::decide_redirect(
              current,
              response,
              history);

      if (!decision.follow)
      {
        co_return response;
      }

      current = http::make_redirect_request(current, decision);
    }
  }

  Response Session::request(
      Method method,
      std::string_view url,
      RequestOptions options,
      Body body)
  {
    return send(Request(
        method,
        url,
        std::move(options),
        std::move(body)));
  }

  Response Session::request(
      std::string_view method,
      std::string_view url,
      RequestOptions options,
      Body body)
  {
    return send(Request(
        method,
        url,
        std::move(options),
        std::move(body)));
  }

  Response Session::get(
      std::string_view url,
      RequestOptions options)
  {
    return request(
        Method::Get,
        url,
        std::move(options));
  }

  Response Session::post(
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    return request(
        Method::Post,
        url,
        std::move(options),
        std::move(body));
  }

  Response Session::put(
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    return request(
        Method::Put,
        url,
        std::move(options),
        std::move(body));
  }

  Response Session::patch(
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    return request(
        Method::Patch,
        url,
        std::move(options),
        std::move(body));
  }

  Response Session::del(
      std::string_view url,
      RequestOptions options)
  {
    return request(
        Method::Delete,
        url,
        std::move(options));
  }

  Response Session::head(
      std::string_view url,
      RequestOptions options)
  {
    return request(
        Method::Head,
        url,
        std::move(options));
  }

  core::task<Response> Session::async_request(
      core::io_context &ctx,
      Method method,
      std::string_view url,
      RequestOptions options,
      Body body)
  {
    auto pending = async_send(
        ctx,
        Request(method, url, std::move(options), std::move(body)));
    co_return co_await pending;
  }

  core::task<Response> Session::async_request(
      core::io_context &ctx,
      std::string_view method,
      std::string_view url,
      RequestOptions options,
      Body body)
  {
    auto pending = async_send(
        ctx,
        Request(method, url, std::move(options), std::move(body)));
    co_return co_await pending;
  }

  core::task<Response> Session::async_get(
      core::io_context &ctx,
      std::string_view url,
      RequestOptions options)
  {
    auto pending = async_request(ctx, Method::Get, url, std::move(options));
    co_return co_await pending;
  }

  core::task<Response> Session::async_post(
      core::io_context &ctx,
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    auto pending = async_request(
        ctx,
        Method::Post,
        url,
        std::move(options),
        std::move(body));
    co_return co_await pending;
  }

  core::task<Response> Session::async_put(
      core::io_context &ctx,
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    auto pending = async_request(
        ctx,
        Method::Put,
        url,
        std::move(options),
        std::move(body));
    co_return co_await pending;
  }

  core::task<Response> Session::async_patch(
      core::io_context &ctx,
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    auto pending = async_request(
        ctx,
        Method::Patch,
        url,
        std::move(options),
        std::move(body));
    co_return co_await pending;
  }

  core::task<Response> Session::async_del(
      core::io_context &ctx,
      std::string_view url,
      RequestOptions options)
  {
    auto pending = async_request(ctx, Method::Delete, url, std::move(options));
    co_return co_await pending;
  }

  core::task<Response> Session::async_head(
      core::io_context &ctx,
      std::string_view url,
      RequestOptions options)
  {
    auto pending = async_request(ctx, Method::Head, url, std::move(options));
    co_return co_await pending;
  }

} // namespace vix::requests

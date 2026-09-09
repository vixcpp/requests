/**
 *
 *  @file Client.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP client implementation for the Vix requests module.
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

#include <vix/requests/Client.hpp>
#include <vix/requests/Error.hpp>
#include <vix/async/core/io_context.hpp>

#include "http/RedirectPolicy.hpp"
#include "transport/TransportFactory.hpp"
#include "transport/ConnectionPool.hpp"

#include <exception>
#include <utility>

namespace vix::requests
{
  namespace
  {
    namespace core = vix::async::core;

    [[nodiscard]] Response send_once(const Request &request, transport::ConnectionPool &pool)
    {
      const Url url = request.final_url();
      auto acquired = pool.acquire(url);
      try { Response response = acquired.transport->send(request); pool.release(url, std::move(acquired.transport)); return response; }
      catch (...)
      {
        const bool retry = acquired.reused &&
            (request.method() == "GET" || request.method() == "HEAD" || request.method() == "OPTIONS");
        pool.discard(std::move(acquired.transport));
        if (!retry) throw;
        auto fresh = pool.acquire(url);
        try { Response response = fresh.transport->send(request); pool.release(url, std::move(fresh.transport)); return response; }
        catch (...) { pool.discard(std::move(fresh.transport)); throw; }
      }
    }

    [[nodiscard]] core::task<Response> async_send_once(
        core::io_context &ctx,
        Request request, transport::ConnectionPool &pool)
    {
      const Url url = request.final_url();
      auto acquired = pool.acquire_async(url, ctx);
      try { Response response = co_await acquired.transport->async_send(ctx, request); pool.release(url, std::move(acquired.transport)); co_return response; }
      catch (...)
      {
        const bool retry = acquired.reused &&
            (request.method() == "GET" || request.method() == "HEAD" || request.method() == "OPTIONS");
        pool.discard(std::move(acquired.transport));
        if (!retry) throw;
      }
      auto fresh = pool.acquire_async(url, ctx);
      try { Response response = co_await fresh.transport->async_send(ctx, std::move(request)); pool.release(url, std::move(fresh.transport)); co_return response; }
      catch (...) { pool.discard(std::move(fresh.transport)); throw; }
    }

  } // namespace

  Client::Client() : pool_(std::make_shared<transport::ConnectionPool>()) {}
  Client::~Client() = default;

  Response Client::send(const Request &request) const
  {
    http::RedirectHistory history;
    Request current = request;

    while (true)
    {
      const std::string currentUrl =
          current.final_url().without_fragment();

      if (history.contains(currentUrl))
      {
        throw TooManyRedirectsException("redirect loop detected");
      }

      history.add(currentUrl);

      Response response = send_once(current, *pool_);

      const http::RedirectDecision decision =
          http::decide_redirect(
              current,
              response,
              history);

      if (!decision.follow)
      {
        return response;
      }

      current = http::make_redirect_request(current, decision);
    }
  }

  core::task<Response> Client::async_send(
      core::io_context &ctx,
      Request request) const
  {
    http::RedirectHistory history;
    Request current = std::move(request);

    while (true)
    {
      const std::string currentUrl =
          current.final_url().without_fragment();

      if (history.contains(currentUrl))
      {
        throw TooManyRedirectsException("redirect loop detected");
      }

      history.add(currentUrl);

      Response response = co_await async_send_once(ctx, current, *pool_);
      const http::RedirectDecision decision =
          http::decide_redirect(current, response, history);

      if (!decision.follow)
      {
        co_return response;
      }

      current = http::make_redirect_request(current, decision);
    }
  }

  Response Client::request(
      Method method,
      std::string_view url,
      RequestOptions options,
      Body body) const
  {
    return send(Request(
        method,
        url,
        std::move(options),
        std::move(body)));
  }

  Response Client::request(
      std::string_view method,
      std::string_view url,
      RequestOptions options,
      Body body) const
  {
    return send(Request(
        method,
        url,
        std::move(options),
        std::move(body)));
  }

  Response Client::get(
      std::string_view url,
      RequestOptions options) const
  {
    return request(
        Method::Get,
        url,
        std::move(options));
  }

  Response Client::post(
      std::string_view url,
      Body body,
      RequestOptions options) const
  {
    return request(
        Method::Post,
        url,
        std::move(options),
        std::move(body));
  }

  Response Client::put(
      std::string_view url,
      Body body,
      RequestOptions options) const
  {
    return request(
        Method::Put,
        url,
        std::move(options),
        std::move(body));
  }

  Response Client::patch(
      std::string_view url,
      Body body,
      RequestOptions options) const
  {
    return request(
        Method::Patch,
        url,
        std::move(options),
        std::move(body));
  }

  Response Client::del(
      std::string_view url,
      RequestOptions options) const
  {
    return request(
        Method::Delete,
        url,
        std::move(options));
  }

  Response Client::head(
      std::string_view url,
      RequestOptions options) const
  {
    return request(
        Method::Head,
        url,
        std::move(options));
  }

  core::task<Response> Client::async_request(
      core::io_context &ctx,
      Method method,
      std::string url,
      RequestOptions options,
      Body body) const
  {
    Request request(method, url, std::move(options), std::move(body));
    auto pending = async_send(
        ctx,
        request);
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_request(
      core::io_context &ctx,
      std::string method,
      std::string url,
      RequestOptions options,
      Body body) const
  {
    Request request(method, url, std::move(options), std::move(body));
    auto pending = async_send(
        ctx,
        request);
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_get(
      core::io_context &ctx,
      std::string url,
      RequestOptions options) const
  {
    auto pending = async_request(ctx, Method::Get, url, std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_post(
      core::io_context &ctx,
      std::string url,
      Body body,
      RequestOptions options) const
  {
    auto pending = async_request(
        ctx,
        Method::Post,
        url,
        std::move(options),
        std::move(body));
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_put(
      core::io_context &ctx,
      std::string url,
      Body body,
      RequestOptions options) const
  {
    auto pending = async_request(
        ctx,
        Method::Put,
        url,
        std::move(options),
        std::move(body));
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_patch(
      core::io_context &ctx,
      std::string url,
      Body body,
      RequestOptions options) const
  {
    auto pending = async_request(
        ctx,
        Method::Patch,
        url,
        std::move(options),
        std::move(body));
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_del(
      core::io_context &ctx,
      std::string url,
      RequestOptions options) const
  {
    auto pending = async_request(ctx, Method::Delete, url, std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> Client::async_head(
      core::io_context &ctx,
      std::string url,
      RequestOptions options) const
  {
    auto pending = async_request(ctx, Method::Head, url, std::move(options));
    co_return co_await std::move(pending);
  }

  Response request(
      Method method,
      std::string_view url,
      RequestOptions options,
      Body body)
  {
    return Client{}.request(
        method,
        url,
        std::move(options),
        std::move(body));
  }

  Response request(
      std::string_view method,
      std::string_view url,
      RequestOptions options,
      Body body)
  {
    return Client{}.request(
        method,
        url,
        std::move(options),
        std::move(body));
  }

  Response get(
      std::string_view url,
      RequestOptions options)
  {
    return Client{}.get(
        url,
        std::move(options));
  }

  Response post(
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    return Client{}.post(
        url,
        std::move(body),
        std::move(options));
  }

  Response put(
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    return Client{}.put(
        url,
        std::move(body),
        std::move(options));
  }

  Response patch(
      std::string_view url,
      Body body,
      RequestOptions options)
  {
    return Client{}.patch(
        url,
        std::move(body),
        std::move(options));
  }

  Response del(
      std::string_view url,
      RequestOptions options)
  {
    return Client{}.del(
        url,
        std::move(options));
  }

  Response head(
      std::string_view url,
      RequestOptions options)
  {
    return Client{}.head(
        url,
        std::move(options));
  }

  core::task<Response> async_request(
      core::io_context &ctx,
      Method method,
      std::string url,
      RequestOptions options,
      Body body)
  {
    Client client;
    auto pending = client.async_request(
        ctx,
        method,
        url,
        std::move(options),
        std::move(body));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_request(
      core::io_context &ctx,
      std::string method,
      std::string url,
      RequestOptions options,
      Body body)
  {
    Client client;
    auto pending = client.async_request(
        ctx,
        method,
        url,
        std::move(options),
        std::move(body));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_get(
      core::io_context &ctx,
      std::string url,
      RequestOptions options)
  {
    Client client;
    auto pending = client.async_get(ctx, url, std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_post(
      core::io_context &ctx,
      std::string url,
      Body body,
      RequestOptions options)
  {
    Client client;
    auto pending = client.async_post(
        ctx,
        url,
        std::move(body),
        std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_put(
      core::io_context &ctx,
      std::string url,
      Body body,
      RequestOptions options)
  {
    Client client;
    auto pending = client.async_put(
        ctx,
        url,
        std::move(body),
        std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_patch(
      core::io_context &ctx,
      std::string url,
      Body body,
      RequestOptions options)
  {
    Client client;
    auto pending = client.async_patch(
        ctx,
        url,
        std::move(body),
        std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_del(
      core::io_context &ctx,
      std::string url,
      RequestOptions options)
  {
    Client client;
    auto pending = client.async_del(ctx, url, std::move(options));
    co_return co_await std::move(pending);
  }

  core::task<Response> async_head(
      core::io_context &ctx,
      std::string url,
      RequestOptions options)
  {
    Client client;
    auto pending = client.async_head(ctx, url, std::move(options));
    co_return co_await std::move(pending);
  }

} // namespace vix::requests

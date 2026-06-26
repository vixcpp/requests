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

#include "http/RedirectPolicy.hpp"
#include "transport/TransportFactory.hpp"

#include <utility>

namespace vix::requests
{
  namespace
  {
    [[nodiscard]] Response send_once(const Request &request)
    {
      auto transport =
          transport::make_transport_for_url(request.final_url());

      return transport->send(request);
    }
  } // namespace

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

      Response response = send_once(current);

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

} // namespace vix::requests

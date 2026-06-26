/**
 *
 *  @file Request.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP request object implementation.
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

#include <vix/requests/Request.hpp>
#include <vix/requests/Error.hpp>

#include <utility>

namespace vix::requests
{
  Request::Request()
      : method_("GET")
  {
  }

  Request::Request(
      Method method,
      std::string_view url,
      RequestOptions options,
      Body body)
      : method_(std::string(to_string(method))),
        url_(Url::parse(url)),
        options_(std::move(options)),
        body_(std::move(body))
  {
  }

  Request::Request(
      std::string_view method,
      std::string_view url,
      RequestOptions options,
      Body body)
      : method_(make_method(method)),
        url_(Url::parse(url)),
        options_(std::move(options)),
        body_(std::move(body))
  {
  }

  const std::string &Request::method() const noexcept
  {
    return method_;
  }

  std::optional<Method> Request::known_method() const
  {
    return method_from_string(method_);
  }

  const Url &Request::url() const noexcept
  {
    return url_;
  }

  Url Request::final_url() const
  {
    return url_.with_params(options_.params);
  }

  std::string Request::request_target() const
  {
    return final_url().request_target();
  }

  const RequestOptions &Request::options() const noexcept
  {
    return options_;
  }

  RequestOptions &Request::options() noexcept
  {
    return options_;
  }

  const Body &Request::body() const noexcept
  {
    return body_;
  }

  void Request::set_body(Body body)
  {
    body_ = std::move(body);
  }

  bool Request::has_body() const noexcept
  {
    return !body_.empty();
  }

  Headers Request::effective_headers() const
  {
    Headers headers = options_.headers;

    if (!headers.has("Host"))
    {
      headers.set("Host", host_header());
    }

    if (!headers.has("User-Agent") && options_.has_user_agent())
    {
      headers.set("User-Agent", options_.user_agent);
    }

    if (!headers.has("Accept"))
    {
      headers.set("Accept", "*/*");
    }

    if (!headers.has("Connection"))
    {
      headers.set(
          "Connection",
          options_.keep_alive ? "keep-alive" : "close");
    }

    if (has_body())
    {
      if (body_.has_content_type() && !headers.has("Content-Type"))
      {
        headers.set("Content-Type", body_.content_type());
      }

      if (!headers.has("Content-Length"))
      {
        headers.set("Content-Length", std::to_string(body_.size()));
      }
    }

    return headers;
  }

  std::string Request::host_header() const
  {
    if (options_.has_host_override())
    {
      return *options_.host_override;
    }

    return final_url().authority();
  }

  bool Request::expects_response_body() const
  {
    const auto known = known_method();

    if (!known.has_value())
    {
      return true;
    }

    return method_expects_response_body(*known);
  }

  bool Request::allows_request_body() const
  {
    const auto known = known_method();

    if (!known.has_value())
    {
      return true;
    }

    return method_allows_request_body(*known);
  }

  std::string Request::make_method(std::string_view method)
  {
    if (!is_valid_method_token(method))
    {
      throw RequestException("invalid HTTP method");
    }

    return normalize_method(method);
  }

} // namespace vix::requests

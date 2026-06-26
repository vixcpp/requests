/**
 *
 *  @file Error.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Request exception implementation for the Vix requests module.
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

#include <sstream>
#include <utility>

namespace vix::requests
{
  RequestException::RequestException(std::string message)
      : std::runtime_error(std::move(message))
  {
  }

  InvalidUrlException::InvalidUrlException(std::string message)
      : RequestException(std::move(message))
  {
  }

  UnsupportedProtocolException::UnsupportedProtocolException(std::string message)
      : RequestException(std::move(message))
  {
  }

  TransportException::TransportException(std::string message)
      : RequestException(std::move(message))
  {
  }

  ConnectionException::ConnectionException(std::string message)
      : TransportException(std::move(message))
  {
  }

  TimeoutException::TimeoutException(std::string message)
      : TransportException(std::move(message))
  {
  }

  TooManyRedirectsException::TooManyRedirectsException(std::string message)
      : RequestException(std::move(message))
  {
  }

  HttpException::HttpException(
      int statusCode,
      std::string reason,
      std::string url)
      : RequestException(make_http_error_message(statusCode, reason, url)),
        statusCode_(statusCode),
        reason_(std::move(reason)),
        url_(std::move(url))
  {
  }

  int HttpException::status_code() const noexcept
  {
    return statusCode_;
  }

  const std::string &HttpException::reason() const noexcept
  {
    return reason_;
  }

  const std::string &HttpException::url() const noexcept
  {
    return url_;
  }

  std::string make_http_error_message(
      int statusCode,
      const std::string &reason,
      const std::string &url)
  {
    std::ostringstream oss;
    oss << "HTTP request failed with status " << statusCode;

    if (!reason.empty())
    {
      oss << " (" << reason << ")";
    }

    if (!url.empty())
    {
      oss << " for " << url;
    }

    return oss.str();
  }

} // namespace vix::requests

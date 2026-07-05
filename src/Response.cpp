/**
 *
 *  @file Response.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP response object implementation.
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

#include <vix/requests/Response.hpp>
#include <vix/requests/Error.hpp>

#include <cctype>
#include <limits>
#include <utility>

namespace vix::requests
{
  namespace
  {
    [[nodiscard]] std::optional<std::size_t> parse_size(
        const std::string &value) noexcept
    {
      if (value.empty())
      {
        return std::nullopt;
      }

      std::size_t result = 0;

      for (char raw_ch : value)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (std::isdigit(ch) == 0)
        {
          return std::nullopt;
        }

        const std::size_t digit = static_cast<std::size_t>(ch - '0');

        if (result >
            (std::numeric_limits<std::size_t>::max() - digit) / 10U)
        {
          return std::nullopt;
        }

        result = (result * 10U) + digit;
      }

      return result;
    }
  } // namespace

  Response::Response(
      std::string url,
      int statusCode,
      std::string reason,
      Headers headers,
      std::string body)
      : url_(std::move(url)),
        statusCode_(statusCode),
        reason_(std::move(reason)),
        headers_(std::move(headers)),
        body_(std::move(body))
  {
    if (reason_.empty())
    {
      reason_ = std::string(default_reason_phrase(statusCode_));
    }
  }

  const std::string &Response::url() const noexcept
  {
    return url_;
  }

  void Response::set_url(std::string value)
  {
    url_ = std::move(value);
  }

  int Response::status_code() const noexcept
  {
    return statusCode_;
  }

  void Response::set_status_code(int value) noexcept
  {
    statusCode_ = value;
  }

  const std::string &Response::reason() const noexcept
  {
    return reason_;
  }

  void Response::set_reason(std::string value)
  {
    reason_ = std::move(value);
  }

  const Headers &Response::headers() const noexcept
  {
    return headers_;
  }

  Headers &Response::headers() noexcept
  {
    return headers_;
  }

  void Response::set_headers(Headers value)
  {
    headers_ = std::move(value);
  }

  const std::string &Response::text() const noexcept
  {
    return body_;
  }

  const std::string &Response::body() const noexcept
  {
    return body_;
  }

  void Response::set_body(std::string value)
  {
    body_ = std::move(value);
  }

  std::vector<unsigned char> Response::bytes() const
  {
    return std::vector<unsigned char>(body_.begin(), body_.end());
  }

  std::size_t Response::size() const noexcept
  {
    return body_.size();
  }

  bool Response::empty() const noexcept
  {
    return body_.empty();
  }

  bool Response::ok() const noexcept
  {
    return statusCode_ >= 200 && statusCode_ < 300;
  }

  bool Response::is_redirect() const noexcept
  {
    return is_redirect_status(statusCode_);
  }

  bool Response::is_error() const noexcept
  {
    return statusCode_ >= 400;
  }

  void Response::raise_for_status() const
  {
    if (is_error())
    {
      throw HttpException(statusCode_, reason_, url_);
    }
  }

  std::optional<std::string> Response::header(
      std::string_view name) const
  {
    return headers_.get(name);
  }

  std::vector<std::string> Response::headers_all(
      std::string_view name) const
  {
    return headers_.get_all(name);
  }

  std::optional<std::string> Response::content_type() const
  {
    return header("Content-Type");
  }

  std::optional<std::size_t> Response::content_length() const
  {
    const auto value = header("Content-Length");

    if (!value.has_value())
    {
      return std::nullopt;
    }

    return parse_size(*value);
  }

  std::optional<std::string> Response::location() const
  {
    return header("Location");
  }

  Response::Duration Response::elapsed() const noexcept
  {
    return elapsed_;
  }

  void Response::set_elapsed(Duration value) noexcept
  {
    elapsed_ = value;
  }

  std::string_view default_reason_phrase(int statusCode) noexcept
  {
    switch (statusCode)
    {
    case 100:
      return "Continue";
    case 101:
      return "Switching Protocols";
    case 102:
      return "Processing";
    case 103:
      return "Early Hints";

    case 200:
      return "OK";
    case 201:
      return "Created";
    case 202:
      return "Accepted";
    case 203:
      return "Non-Authoritative Information";
    case 204:
      return "No Content";
    case 205:
      return "Reset Content";
    case 206:
      return "Partial Content";

    case 300:
      return "Multiple Choices";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 303:
      return "See Other";
    case 304:
      return "Not Modified";
    case 307:
      return "Temporary Redirect";
    case 308:
      return "Permanent Redirect";

    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 402:
      return "Payment Required";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 406:
      return "Not Acceptable";
    case 407:
      return "Proxy Authentication Required";
    case 408:
      return "Request Timeout";
    case 409:
      return "Conflict";
    case 410:
      return "Gone";
    case 411:
      return "Length Required";
    case 412:
      return "Precondition Failed";
    case 413:
      return "Content Too Large";
    case 414:
      return "URI Too Long";
    case 415:
      return "Unsupported Media Type";
    case 416:
      return "Range Not Satisfiable";
    case 417:
      return "Expectation Failed";
    case 418:
      return "I'm a teapot";
    case 421:
      return "Misdirected Request";
    case 422:
      return "Unprocessable Content";
    case 423:
      return "Locked";
    case 424:
      return "Failed Dependency";
    case 425:
      return "Too Early";
    case 426:
      return "Upgrade Required";
    case 428:
      return "Precondition Required";
    case 429:
      return "Too Many Requests";
    case 431:
      return "Request Header Fields Too Large";
    case 451:
      return "Unavailable For Legal Reasons";

    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implemented";
    case 502:
      return "Bad Gateway";
    case 503:
      return "Service Unavailable";
    case 504:
      return "Gateway Timeout";
    case 505:
      return "HTTP Version Not Supported";
    case 506:
      return "Variant Also Negotiates";
    case 507:
      return "Insufficient Storage";
    case 508:
      return "Loop Detected";
    case 510:
      return "Not Extended";
    case 511:
      return "Network Authentication Required";

    default:
      return "";
    }
  }

  bool is_redirect_status(int statusCode) noexcept
  {
    return statusCode == 301 ||
           statusCode == 302 ||
           statusCode == 303 ||
           statusCode == 307 ||
           statusCode == 308;
  }

} // namespace vix::requests

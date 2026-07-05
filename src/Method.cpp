/**
 *
 *  @file Method.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP method helper implementation for the Vix requests module.
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

#include <vix/requests/Method.hpp>

#include <cctype>

namespace vix::requests
{
  namespace
  {
    [[nodiscard]] bool is_token_char(unsigned char ch) noexcept
    {
      if (std::isalnum(ch) != 0)
      {
        return true;
      }

      switch (ch)
      {
      case '!':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '*':
      case '+':
      case '-':
      case '.':
      case '^':
      case '_':
      case '`':
      case '|':
      case '~':
        return true;

      default:
        return false;
      }
    }
  } // namespace

  std::string_view to_string(Method method) noexcept
  {
    switch (method)
    {
    case Method::Get:
      return "GET";

    case Method::Post:
      return "POST";

    case Method::Put:
      return "PUT";

    case Method::Patch:
      return "PATCH";

    case Method::Delete:
      return "DELETE";

    case Method::Head:
      return "HEAD";

    case Method::Options:
      return "OPTIONS";

    case Method::Trace:
      return "TRACE";

    case Method::Connect:
      return "CONNECT";
    }

    return "GET";
  }

  std::optional<Method> method_from_string(std::string_view value)
  {
    if (!is_valid_method_token(value))
    {
      return std::nullopt;
    }

    const std::string normalized = normalize_method(value);

    if (normalized == "GET")
    {
      return Method::Get;
    }

    if (normalized == "POST")
    {
      return Method::Post;
    }

    if (normalized == "PUT")
    {
      return Method::Put;
    }

    if (normalized == "PATCH")
    {
      return Method::Patch;
    }

    if (normalized == "DELETE")
    {
      return Method::Delete;
    }

    if (normalized == "HEAD")
    {
      return Method::Head;
    }

    if (normalized == "OPTIONS")
    {
      return Method::Options;
    }

    if (normalized == "TRACE")
    {
      return Method::Trace;
    }

    if (normalized == "CONNECT")
    {
      return Method::Connect;
    }

    return std::nullopt;
  }

  std::string normalize_method(std::string_view value)
  {
    std::string normalized;
    normalized.reserve(value.size());

    for (char raw_ch : value)
    {
      const auto ch = static_cast<unsigned char>(raw_ch);
      normalized.push_back(
          static_cast<char>(std::toupper(ch)));
    }

    return normalized;
  }

  bool is_valid_method_token(std::string_view value) noexcept
  {
    if (value.empty())
    {
      return false;
    }

    for (char raw_ch : value)
    {
      const auto ch = static_cast<unsigned char>(raw_ch);
      if (!is_token_char(ch))
      {
        return false;
      }
    }

    return true;
  }

  bool method_allows_request_body(Method method) noexcept
  {
    switch (method)
    {
    case Method::Post:
    case Method::Put:
    case Method::Patch:
    case Method::Delete:
      return true;

    case Method::Get:
    case Method::Head:
    case Method::Options:
    case Method::Trace:
    case Method::Connect:
      return false;
    }

    return false;
  }

  bool method_expects_response_body(Method method) noexcept
  {
    return method != Method::Head;
  }

} // namespace vix::requests

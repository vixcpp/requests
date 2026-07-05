/**
 *
 *  @file CaseInsensitive.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Case-insensitive string helper implementation.
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

#include "detail/CaseInsensitive.hpp"

#include <cctype>

namespace vix::requests::detail
{
  std::string ascii_to_lower(std::string_view value)
  {
    std::string result;
    result.reserve(value.size());

    for (char raw_ch : value)
    {
      const auto ch = static_cast<unsigned char>(raw_ch);
      result.push_back(static_cast<char>(std::tolower(ch)));
    }

    return result;
  }

  bool ascii_iequals(
      std::string_view lhs,
      std::string_view rhs) noexcept
  {
    if (lhs.size() != rhs.size())
    {
      return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
      const auto left = static_cast<unsigned char>(lhs[index]);
      const auto right = static_cast<unsigned char>(rhs[index]);

      if (std::tolower(left) != std::tolower(right))
      {
        return false;
      }
    }

    return true;
  }

  bool is_http_token_char(unsigned char ch) noexcept
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

  bool is_http_token(std::string_view value) noexcept
  {
    if (value.empty())
    {
      return false;
    }

    for (char raw_ch : value)
    {
      const auto ch = static_cast<unsigned char>(raw_ch);
      if (!is_http_token_char(ch))
      {
        return false;
      }
    }

    return true;
  }

  std::string trim_ows(std::string_view value)
  {
    std::size_t start = 0;
    while (start < value.size() &&
           (value[start] == ' ' || value[start] == '\t'))
    {
      ++start;
    }

    std::size_t end = value.size();
    while (end > start &&
           (value[end - 1] == ' ' || value[end - 1] == '\t'))
    {
      --end;
    }

    return std::string(value.substr(start, end - start));
  }

} // namespace vix::requests::detail

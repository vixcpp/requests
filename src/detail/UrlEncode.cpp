/**
 *
 *  @file UrlEncode.cpp
 *  @author Gaspard Kirira
 *
 *  @brief URL encoding helper implementation.
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

#include "detail/UrlEncode.hpp"

#include <cctype>

namespace vix::requests::detail
{
  namespace
  {
    [[nodiscard]] bool is_unreserved(unsigned char ch) noexcept
    {
      return std::isalnum(ch) != 0 ||
             ch == '-' ||
             ch == '_' ||
             ch == '.' ||
             ch == '~';
    }

    [[nodiscard]] char from_hex(unsigned char ch) noexcept
    {
      if (ch >= '0' && ch <= '9')
      {
        return static_cast<char>(ch - '0');
      }

      if (ch >= 'a' && ch <= 'f')
      {
        return static_cast<char>(ch - 'a' + 10);
      }

      if (ch >= 'A' && ch <= 'F')
      {
        return static_cast<char>(ch - 'A' + 10);
      }

      return -1;
    }

    [[nodiscard]] std::string encode_impl(
        std::string_view value,
        bool spaceAsPlus)
    {
      static constexpr char hex[] = "0123456789ABCDEF";

      std::string encoded;
      encoded.reserve(value.size() * 3);

      for (char raw_ch : value)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (is_unreserved(ch))
        {
          encoded.push_back(static_cast<char>(ch));
          continue;
        }

        if (spaceAsPlus && ch == ' ')
        {
          encoded.push_back('+');
          continue;
        }

        encoded.push_back('%');
        encoded.push_back(hex[(ch >> 4U) & 0x0FU]);
        encoded.push_back(hex[ch & 0x0FU]);
      }

      return encoded;
    }

    [[nodiscard]] std::string decode_impl(
        std::string_view value,
        bool plusAsSpace)
    {
      std::string decoded;
      decoded.reserve(value.size());

      for (std::size_t index = 0; index < value.size(); ++index)
      {
        const unsigned char ch = static_cast<unsigned char>(value[index]);

        if (plusAsSpace && ch == '+')
        {
          decoded.push_back(' ');
          continue;
        }

        if (ch == '%' && index + 2 < value.size())
        {
          const char high = from_hex(
              static_cast<unsigned char>(value[index + 1]));
          const char low = from_hex(
              static_cast<unsigned char>(value[index + 2]));

          if (high >= 0 && low >= 0)
          {
            decoded.push_back(
                static_cast<char>((high << 4) | low));
            index += 2;
            continue;
          }
        }

        decoded.push_back(static_cast<char>(ch));
      }

      return decoded;
    }
  } // namespace

  std::string url_encode_component(std::string_view value)
  {
    return encode_impl(value, false);
  }

  std::string form_url_encode_component(std::string_view value)
  {
    return encode_impl(value, true);
  }

  std::string url_decode_component(std::string_view value)
  {
    return decode_impl(value, false);
  }

  std::string form_url_decode_component(std::string_view value)
  {
    return decode_impl(value, true);
  }

} // namespace vix::requests::detail

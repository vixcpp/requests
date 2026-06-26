/**
 *
 *  @file Base64.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Base64 helper implementation.
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

#include "detail/Base64.hpp"

namespace vix::requests::detail
{
  namespace
  {
    constexpr char base64Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    [[nodiscard]] std::string base64_encode_bytes(
        const unsigned char *data,
        std::size_t size)
    {
      std::string encoded;
      encoded.reserve(((size + 2U) / 3U) * 4U);

      for (std::size_t index = 0; index < size; index += 3U)
      {
        const unsigned int octetA = data[index];
        const unsigned int octetB =
            index + 1U < size ? data[index + 1U] : 0U;
        const unsigned int octetC =
            index + 2U < size ? data[index + 2U] : 0U;

        const unsigned int triple =
            (octetA << 16U) |
            (octetB << 8U) |
            octetC;

        encoded.push_back(base64Alphabet[(triple >> 18U) & 0x3FU]);
        encoded.push_back(base64Alphabet[(triple >> 12U) & 0x3FU]);

        if (index + 1U < size)
        {
          encoded.push_back(base64Alphabet[(triple >> 6U) & 0x3FU]);
        }
        else
        {
          encoded.push_back('=');
        }

        if (index + 2U < size)
        {
          encoded.push_back(base64Alphabet[triple & 0x3FU]);
        }
        else
        {
          encoded.push_back('=');
        }
      }

      return encoded;
    }
  } // namespace

  std::string base64_encode(const std::vector<unsigned char> &data)
  {
    if (data.empty())
    {
      return {};
    }

    return base64_encode_bytes(data.data(), data.size());
  }

  std::string base64_encode(std::string_view data)
  {
    if (data.empty())
    {
      return {};
    }

    return base64_encode_bytes(
        reinterpret_cast<const unsigned char *>(data.data()),
        data.size());
  }

  std::string make_basic_auth_value(
      std::string_view username,
      std::string_view password)
  {
    std::string credentials;
    credentials.reserve(username.size() + 1U + password.size());

    credentials.append(username);
    credentials.push_back(':');
    credentials.append(password);

    return "Basic " + base64_encode(credentials);
  }

} // namespace vix::requests::detail

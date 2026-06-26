/**
 *
 *  @file Base64.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Base64 helpers for the Vix requests module.
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

#ifndef VIX_REQUESTS_DETAIL_BASE64_HPP
#define VIX_REQUESTS_DETAIL_BASE64_HPP

#include <string>
#include <string_view>
#include <vector>

namespace vix::requests::detail
{
  /**
   * @brief Encodes binary data as Base64.
   *
   * @param data Input bytes.
   * @return Base64 encoded string.
   */
  [[nodiscard]] std::string base64_encode(
      const std::vector<unsigned char> &data);

  /**
   * @brief Encodes string data as Base64.
   *
   * @param data Input data.
   * @return Base64 encoded string.
   */
  [[nodiscard]] std::string base64_encode(std::string_view data);

  /**
   * @brief Builds the Basic Authorization header value.
   *
   * @param username Basic auth username.
   * @param password Basic auth password.
   * @return Authorization header value.
   */
  [[nodiscard]] std::string make_basic_auth_value(
      std::string_view username,
      std::string_view password);

} // namespace vix::requests::detail

#endif // VIX_REQUESTS_DETAIL_BASE64_HPP

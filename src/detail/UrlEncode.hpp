/**
 *
 *  @file UrlEncode.hpp
 *  @author Gaspard Kirira
 *
 *  @brief URL encoding helpers for the Vix requests module.
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

#ifndef VIX_REQUESTS_DETAIL_URL_ENCODE_HPP
#define VIX_REQUESTS_DETAIL_URL_ENCODE_HPP

#include <string>
#include <string_view>

namespace vix::requests::detail
{
  /**
   * @brief Percent-encodes a URL component.
   *
   * Spaces are encoded as %20. This is suitable for query keys, query values,
   * paths, and generic URL components.
   *
   * @param value Input value.
   * @return Percent-encoded value.
   */
  [[nodiscard]] std::string url_encode_component(std::string_view value);

  /**
   * @brief Percent-encodes a form component.
   *
   * Spaces are encoded as '+'. This is suitable for
   * application/x-www-form-urlencoded bodies.
   *
   * @param value Input value.
   * @return Form-encoded value.
   */
  [[nodiscard]] std::string form_url_encode_component(std::string_view value);

  /**
   * @brief Decodes a percent-encoded URL component.
   *
   * Invalid percent sequences are preserved as-is.
   *
   * @param value Encoded value.
   * @return Decoded value.
   */
  [[nodiscard]] std::string url_decode_component(std::string_view value);

  /**
   * @brief Decodes a form-url-encoded component.
   *
   * '+' is decoded as space.
   *
   * @param value Encoded value.
   * @return Decoded value.
   */
  [[nodiscard]] std::string form_url_decode_component(std::string_view value);

} // namespace vix::requests::detail

#endif // VIX_REQUESTS_DETAIL_URL_ENCODE_HPP

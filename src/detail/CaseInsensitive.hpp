/**
 *
 *  @file CaseInsensitive.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Case-insensitive string helpers for the Vix requests module.
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

#ifndef VIX_REQUESTS_DETAIL_CASE_INSENSITIVE_HPP
#define VIX_REQUESTS_DETAIL_CASE_INSENSITIVE_HPP

#include <string>
#include <string_view>

namespace vix::requests::detail
{
  /**
   * @brief Converts ASCII characters to lowercase.
   *
   * @param value Input string.
   * @return Lowercase string.
   */
  [[nodiscard]] std::string ascii_to_lower(std::string_view value);

  /**
   * @brief Compares two ASCII strings without case sensitivity.
   *
   * @param lhs Left value.
   * @param rhs Right value.
   * @return True when both values are equal ignoring ASCII case.
   */
  [[nodiscard]] bool ascii_iequals(
      std::string_view lhs,
      std::string_view rhs) noexcept;

  /**
   * @brief Checks whether a character is valid inside an HTTP token.
   *
   * @param ch Character.
   * @return True when the character is valid.
   */
  [[nodiscard]] bool is_http_token_char(unsigned char ch) noexcept;

  /**
   * @brief Checks whether a string is a valid HTTP token.
   *
   * @param value Input string.
   * @return True when value is a valid token.
   */
  [[nodiscard]] bool is_http_token(std::string_view value) noexcept;

  /**
   * @brief Trims optional whitespace used around header values.
   *
   * @param value Input string.
   * @return Trimmed string.
   */
  [[nodiscard]] std::string trim_ows(std::string_view value);

} // namespace vix::requests::detail

#endif // VIX_REQUESTS_DETAIL_CASE_INSENSITIVE_HPP

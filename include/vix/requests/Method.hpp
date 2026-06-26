/**
 *
 *  @file Method.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP method helpers for the Vix requests module.
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

#ifndef VIX_REQUESTS_METHOD_HPP
#define VIX_REQUESTS_METHOD_HPP

#include <optional>
#include <string>
#include <string_view>

namespace vix::requests
{
  /**
   * @brief Known HTTP methods supported by the high-level API.
   */
  enum class Method
  {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Head,
    Options,
    Trace,
    Connect
  };

  /**
   * @brief Converts a known HTTP method to its wire string.
   *
   * @param method HTTP method.
   * @return Stable uppercase method name.
   */
  [[nodiscard]] std::string_view to_string(Method method) noexcept;

  /**
   * @brief Parses a known HTTP method from a string.
   *
   * The parser is case-insensitive. Unknown but syntactically valid custom
   * methods return std::nullopt.
   *
   * @param value Method string.
   * @return Parsed method when it is a known method.
   */
  [[nodiscard]] std::optional<Method> method_from_string(
      std::string_view value);

  /**
   * @brief Normalizes a method string for HTTP serialization.
   *
   * @param value Method string.
   * @return Uppercase method string.
   */
  [[nodiscard]] std::string normalize_method(std::string_view value);

  /**
   * @brief Checks whether a method string is a valid HTTP token.
   *
   * @param value Method string.
   * @return True when the method can be serialized as an HTTP method token.
   */
  [[nodiscard]] bool is_valid_method_token(std::string_view value) noexcept;

  /**
   * @brief Checks whether a known method commonly allows a request body.
   *
   * @param method HTTP method.
   * @return True when the method commonly carries a request body.
   */
  [[nodiscard]] bool method_allows_request_body(Method method) noexcept;

  /**
   * @brief Checks whether a known method expects a response body.
   *
   * HEAD responses must not expose a body to the caller.
   *
   * @param method HTTP method.
   * @return True when the method can expose a response body.
   */
  [[nodiscard]] bool method_expects_response_body(Method method) noexcept;

} // namespace vix::requests

#endif // VIX_REQUESTS_METHOD_HPP

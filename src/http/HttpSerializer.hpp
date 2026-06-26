/**
 *
 *  @file HttpSerializer.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP request serializer for the Vix requests module.
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

#ifndef VIX_REQUESTS_HTTP_HTTP_SERIALIZER_HPP
#define VIX_REQUESTS_HTTP_HTTP_SERIALIZER_HPP

#include <vix/requests/Headers.hpp>
#include <vix/requests/Request.hpp>

#include <string>
#include <string_view>

namespace vix::requests::http
{
  /**
   * @brief Serialized HTTP request data.
   */
  struct SerializedRequest
  {
    /**
     * @brief Request head: request line + headers + final CRLF.
     */
    std::string head;

    /**
     * @brief Request body.
     */
    std::string body;

    /**
     * @brief Full request bytes.
     */
    std::string data;
  };

  /**
   * @brief Serializes a request into HTTP/1.1 bytes.
   *
   * @param request Prepared request.
   * @return Serialized request.
   */
  [[nodiscard]] SerializedRequest serialize_request(const Request &request);

  /**
   * @brief Serializes only the request head.
   *
   * @param request Prepared request.
   * @return Request head.
   */
  [[nodiscard]] std::string serialize_request_head(const Request &request);

  /**
   * @brief Serializes a single header line.
   *
   * @param name Header name.
   * @param value Header value.
   * @return Header line ending with CRLF.
   */
  [[nodiscard]] std::string serialize_header_line(
      std::string_view name,
      std::string_view value);

  /**
   * @brief Serializes all headers.
   *
   * @param headers Headers.
   * @return Header block.
   */
  [[nodiscard]] std::string serialize_headers(const Headers &headers);

} // namespace vix::requests::http

#endif // VIX_REQUESTS_HTTP_HTTP_SERIALIZER_HPP

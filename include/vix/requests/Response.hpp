/**
 *
 *  @file Response.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP response object for the Vix requests module.
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

#ifndef VIX_REQUESTS_RESPONSE_HPP
#define VIX_REQUESTS_RESPONSE_HPP

#include <vix/requests/Headers.hpp>
#include <vix/requests/Url.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vix::requests
{
  /**
   * @brief HTTP response container.
   */
  class Response
  {
  public:
    using Duration = std::chrono::milliseconds;

    /**
     * @brief Creates an empty response.
     */
    Response() = default;

    /**
     * @brief Creates a response.
     *
     * @param url Final response URL.
     * @param statusCode HTTP status code.
     * @param reason HTTP reason phrase.
     * @param headers Response headers.
     * @param body Response body.
     */
    Response(
        std::string url,
        int statusCode,
        std::string reason = {},
        Headers headers = {},
        std::string body = {});

    /**
     * @brief Returns the final response URL.
     *
     * @return URL.
     */
    [[nodiscard]] const std::string &url() const noexcept;

    /**
     * @brief Sets the final response URL.
     *
     * @param value URL.
     */
    void set_url(std::string value);

    /**
     * @brief Returns the HTTP status code.
     *
     * @return Status code.
     */
    [[nodiscard]] int status_code() const noexcept;

    /**
     * @brief Sets the HTTP status code.
     *
     * @param value Status code.
     */
    void set_status_code(int value) noexcept;

    /**
     * @brief Returns the HTTP reason phrase.
     *
     * @return Reason phrase.
     */
    [[nodiscard]] const std::string &reason() const noexcept;

    /**
     * @brief Sets the HTTP reason phrase.
     *
     * @param value Reason phrase.
     */
    void set_reason(std::string value);

    /**
     * @brief Returns response headers.
     *
     * @return Headers.
     */
    [[nodiscard]] const Headers &headers() const noexcept;

    /**
     * @brief Returns mutable response headers.
     *
     * @return Headers.
     */
    [[nodiscard]] Headers &headers() noexcept;

    /**
     * @brief Replaces response headers.
     *
     * @param value Headers.
     */
    void set_headers(Headers value);

    /**
     * @brief Returns the response body as text.
     *
     * The returned string may contain binary data.
     *
     * @return Response body.
     */
    [[nodiscard]] const std::string &text() const noexcept;

    /**
     * @brief Returns the response body data.
     *
     * @return Response body.
     */
    [[nodiscard]] const std::string &body() const noexcept;

    /**
     * @brief Sets the response body.
     *
     * @param value Body data.
     */
    void set_body(std::string value);

    /**
     * @brief Returns the response body as bytes.
     *
     * @return Response bytes.
     */
    [[nodiscard]] std::vector<unsigned char> bytes() const;

    /**
     * @brief Returns the response body size.
     *
     * @return Body size.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Checks whether the response body is empty.
     *
     * @return True when body is empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns true when the status code is in the 2xx range.
     *
     * @return True for 2xx responses.
     */
    [[nodiscard]] bool ok() const noexcept;

    /**
     * @brief Returns true when the response is a redirect status.
     *
     * @return True for 301, 302, 303, 307, or 308.
     */
    [[nodiscard]] bool is_redirect() const noexcept;

    /**
     * @brief Returns true when the response is an HTTP error status.
     *
     * @return True for status code >= 400.
     */
    [[nodiscard]] bool is_error() const noexcept;

    /**
     * @brief Throws HttpException when the response is an HTTP error.
     */
    void raise_for_status() const;

    /**
     * @brief Returns the first header matching a name.
     *
     * @param name Header name.
     * @return Header value when found.
     */
    [[nodiscard]] std::optional<std::string> header(
        std::string_view name) const;

    /**
     * @brief Returns all headers matching a name.
     *
     * @param name Header name.
     * @return Header values.
     */
    [[nodiscard]] std::vector<std::string> headers_all(
        std::string_view name) const;

    /**
     * @brief Returns the Content-Type header.
     *
     * @return Content-Type value when present.
     */
    [[nodiscard]] std::optional<std::string> content_type() const;

    /**
     * @brief Returns the Content-Length header as a number when valid.
     *
     * @return Content length.
     */
    [[nodiscard]] std::optional<std::size_t> content_length() const;

    /**
     * @brief Returns the Location header.
     *
     * @return Location value when present.
     */
    [[nodiscard]] std::optional<std::string> location() const;

    /**
     * @brief Returns elapsed request time.
     *
     * @return Elapsed duration.
     */
    [[nodiscard]] Duration elapsed() const noexcept;

    /**
     * @brief Sets elapsed request time.
     *
     * @param value Elapsed duration.
     */
    void set_elapsed(Duration value) noexcept;

  private:
    /**
     * @brief Final response URL.
     */
    std::string url_;

    /**
     * @brief HTTP status code.
     */
    int statusCode_{0};

    /**
     * @brief HTTP reason phrase.
     */
    std::string reason_;

    /**
     * @brief Response headers.
     */
    Headers headers_;

    /**
     * @brief Response body.
     */
    std::string body_;

    /**
     * @brief Elapsed request time.
     */
    Duration elapsed_{Duration{0}};
  };

  /**
   * @brief Converts a status code to a common reason phrase.
   *
   * @param statusCode HTTP status code.
   * @return Reason phrase.
   */
  [[nodiscard]] std::string_view default_reason_phrase(
      int statusCode) noexcept;

  /**
   * @brief Checks whether a status code is a redirect status.
   *
   * @param statusCode HTTP status code.
   * @return True for redirect statuses handled by the client.
   */
  [[nodiscard]] bool is_redirect_status(int statusCode) noexcept;

} // namespace vix::requests

#endif // VIX_REQUESTS_RESPONSE_HPP

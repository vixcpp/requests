/**
 *
 *  @file Request.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP request object for the Vix requests module.
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

#ifndef VIX_REQUESTS_REQUEST_HPP
#define VIX_REQUESTS_REQUEST_HPP

#include <vix/requests/Body.hpp>
#include <vix/requests/Headers.hpp>
#include <vix/requests/Method.hpp>
#include <vix/requests/RequestOptions.hpp>
#include <vix/requests/Url.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace vix::requests
{
  /**
   * @brief Prepared HTTP request.
   */
  class Request
  {
  public:
    /**
     * @brief Creates a GET request for an empty URL.
     */
    Request();

    /**
     * @brief Creates a request from a known HTTP method.
     *
     * @param method HTTP method.
     * @param url Target URL.
     * @param options Request options.
     * @param body Request body.
     */
    Request(
        Method method,
        std::string_view url,
        RequestOptions options = {},
        Body body = {});

    /**
     * @brief Creates a request from a custom HTTP method string.
     *
     * @param method HTTP method string.
     * @param url Target URL.
     * @param options Request options.
     * @param body Request body.
     */
    Request(
        std::string_view method,
        std::string_view url,
        RequestOptions options = {},
        Body body = {});

    /**
     * @brief Returns the normalized method string.
     *
     * @return Method string.
     */
    [[nodiscard]] const std::string &method() const noexcept;

    /**
     * @brief Returns the known method when the method is known.
     *
     * @return Known method.
     */
    [[nodiscard]] std::optional<Method> known_method() const;

    /**
     * @brief Returns the base URL without request options params applied.
     *
     * @return Parsed URL.
     */
    [[nodiscard]] const Url &url() const noexcept;

    /**
     * @brief Returns the URL with request options params applied.
     *
     * @return Final URL.
     */
    [[nodiscard]] Url final_url() const;

    /**
     * @brief Returns the HTTP request target.
     *
     * @return Path and query.
     */
    [[nodiscard]] std::string request_target() const;

    /**
     * @brief Returns the request options.
     *
     * @return Request options.
     */
    [[nodiscard]] const RequestOptions &options() const noexcept;

    /**
     * @brief Returns mutable request options.
     *
     * @return Request options.
     */
    [[nodiscard]] RequestOptions &options() noexcept;

    /**
     * @brief Returns the request body.
     *
     * @return Request body.
     */
    [[nodiscard]] const Body &body() const noexcept;

    /**
     * @brief Replaces the request body.
     *
     * @param body Request body.
     */
    void set_body(Body body);

    /**
     * @brief Returns true when the request has a body.
     *
     * @return True when body is not empty.
     */
    [[nodiscard]] bool has_body() const noexcept;

    /**
     * @brief Returns effective headers for serialization.
     *
     * This adds Host, User-Agent, Content-Type, Content-Length, and Connection
     * when they are not already present.
     *
     * @return Effective headers.
     */
    [[nodiscard]] Headers effective_headers() const;

    /**
     * @brief Returns the Host header value.
     *
     * @return Host value.
     */
    [[nodiscard]] std::string host_header() const;

    /**
     * @brief Checks whether this request method can expose a response body.
     *
     * @return False for HEAD, true otherwise.
     */
    [[nodiscard]] bool expects_response_body() const;

    /**
     * @brief Checks whether this request method commonly allows a body.
     *
     * @return True when body is allowed.
     */
    [[nodiscard]] bool allows_request_body() const;

  private:
    /**
     * @brief Normalized HTTP method string.
     */
    std::string method_;

    /**
     * @brief Parsed target URL.
     */
    Url url_;

    /**
     * @brief Request options.
     */
    RequestOptions options_;

    /**
     * @brief Request body.
     */
    Body body_;

    /**
     * @brief Validates and normalizes a method string.
     *
     * @param method Method string.
     * @return Normalized method.
     */
    [[nodiscard]] static std::string make_method(std::string_view method);
  };

} // namespace vix::requests

#endif // VIX_REQUESTS_REQUEST_HPP

/**
 *
 *  @file Client.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP client for the Vix requests module.
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

#ifndef VIX_REQUESTS_CLIENT_HPP
#define VIX_REQUESTS_CLIENT_HPP

#include <vix/requests/Body.hpp>
#include <vix/requests/Method.hpp>
#include <vix/requests/Request.hpp>
#include <vix/requests/RequestOptions.hpp>
#include <vix/requests/Response.hpp>
#include <vix/async/core/task.hpp>

#include <string_view>

namespace vix::async::core
{
  class io_context;
}

namespace vix::requests
{
  /**
   * @brief Stateless HTTP client.
   *
   * Client sends prepared requests, applies redirect behavior, and delegates
   * network I/O to the transport layer. Persistent cookies and reusable
   * defaults are handled by Session.
   */
  class Client
  {
  public:
    /**
     * @brief Creates a client.
     */
    Client() = default;

    /**
     * @brief Sends a prepared request.
     *
     * @param request Prepared request.
     * @return HTTP response.
     */
    [[nodiscard]] Response send(const Request &request) const;

    /**
     * @brief Asynchronously sends a prepared request.
     *
     * @param ctx Async runtime context.
     * @param request Prepared request.
     * @return Task producing the HTTP response.
     */
    [[nodiscard]] vix::async::core::task<Response> async_send(
        vix::async::core::io_context &ctx,
        const Request &request) const;

    /**
     * @brief Sends a request with a known method.
     *
     * @param method HTTP method.
     * @param url Target URL.
     * @param options Request options.
     * @param body Request body.
     * @return HTTP response.
     */
    [[nodiscard]] Response request(
        Method method,
        std::string_view url,
        RequestOptions options = {},
        Body body = {}) const;

    /**
     * @brief Sends a request with a custom method string.
     *
     * @param method HTTP method string.
     * @param url Target URL.
     * @param options Request options.
     * @param body Request body.
     * @return HTTP response.
     */
    [[nodiscard]] Response request(
        std::string_view method,
        std::string_view url,
        RequestOptions options = {},
        Body body = {}) const;

    /**
     * @brief Sends a GET request.
     *
     * @param url Target URL.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response get(
        std::string_view url,
        RequestOptions options = {}) const;

    /**
     * @brief Sends a POST request.
     *
     * @param url Target URL.
     * @param body Request body.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response post(
        std::string_view url,
        Body body = {},
        RequestOptions options = {}) const;

    /**
     * @brief Sends a PUT request.
     *
     * @param url Target URL.
     * @param body Request body.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response put(
        std::string_view url,
        Body body = {},
        RequestOptions options = {}) const;

    /**
     * @brief Sends a PATCH request.
     *
     * @param url Target URL.
     * @param body Request body.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response patch(
        std::string_view url,
        Body body = {},
        RequestOptions options = {}) const;

    /**
     * @brief Sends a DELETE request.
     *
     * @param url Target URL.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response del(
        std::string_view url,
        RequestOptions options = {}) const;

    /**
     * @brief Sends a HEAD request.
     *
     * @param url Target URL.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response head(
        std::string_view url,
        RequestOptions options = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_request(
        vix::async::core::io_context &ctx,
        Method method,
        std::string_view url,
        RequestOptions options = {},
        Body body = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_request(
        vix::async::core::io_context &ctx,
        std::string_view method,
        std::string_view url,
        RequestOptions options = {},
        Body body = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_get(
        vix::async::core::io_context &ctx,
        std::string_view url,
        RequestOptions options = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_post(
        vix::async::core::io_context &ctx,
        std::string_view url,
        Body body = {},
        RequestOptions options = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_put(
        vix::async::core::io_context &ctx,
        std::string_view url,
        Body body = {},
        RequestOptions options = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_patch(
        vix::async::core::io_context &ctx,
        std::string_view url,
        Body body = {},
        RequestOptions options = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_del(
        vix::async::core::io_context &ctx,
        std::string_view url,
        RequestOptions options = {}) const;

    [[nodiscard]] vix::async::core::task<Response> async_head(
        vix::async::core::io_context &ctx,
        std::string_view url,
        RequestOptions options = {}) const;
  };

  /**
   * @brief Sends a generic request with a known method.
   */
  [[nodiscard]] Response request(
      Method method,
      std::string_view url,
      RequestOptions options = {},
      Body body = {});

  /**
   * @brief Sends a generic request with a custom method string.
   */
  [[nodiscard]] Response request(
      std::string_view method,
      std::string_view url,
      RequestOptions options = {},
      Body body = {});

  /**
   * @brief Sends a GET request.
   */
  [[nodiscard]] Response get(
      std::string_view url,
      RequestOptions options = {});

  /**
   * @brief Sends a POST request.
   */
  [[nodiscard]] Response post(
      std::string_view url,
      Body body = {},
      RequestOptions options = {});

  /**
   * @brief Sends a PUT request.
   */
  [[nodiscard]] Response put(
      std::string_view url,
      Body body = {},
      RequestOptions options = {});

  /**
   * @brief Sends a PATCH request.
   */
  [[nodiscard]] Response patch(
      std::string_view url,
      Body body = {},
      RequestOptions options = {});

  /**
   * @brief Sends a DELETE request.
   */
  [[nodiscard]] Response del(
      std::string_view url,
      RequestOptions options = {});

  /**
   * @brief Sends a HEAD request.
   */
  [[nodiscard]] Response head(
      std::string_view url,
      RequestOptions options = {});

  [[nodiscard]] vix::async::core::task<Response> async_request(
      vix::async::core::io_context &ctx,
      Method method,
      std::string_view url,
      RequestOptions options = {},
      Body body = {});

  [[nodiscard]] vix::async::core::task<Response> async_request(
      vix::async::core::io_context &ctx,
      std::string_view method,
      std::string_view url,
      RequestOptions options = {},
      Body body = {});

  [[nodiscard]] vix::async::core::task<Response> async_get(
      vix::async::core::io_context &ctx,
      std::string_view url,
      RequestOptions options = {});

  [[nodiscard]] vix::async::core::task<Response> async_post(
      vix::async::core::io_context &ctx,
      std::string_view url,
      Body body = {},
      RequestOptions options = {});

  [[nodiscard]] vix::async::core::task<Response> async_put(
      vix::async::core::io_context &ctx,
      std::string_view url,
      Body body = {},
      RequestOptions options = {});

  [[nodiscard]] vix::async::core::task<Response> async_patch(
      vix::async::core::io_context &ctx,
      std::string_view url,
      Body body = {},
      RequestOptions options = {});

  [[nodiscard]] vix::async::core::task<Response> async_del(
      vix::async::core::io_context &ctx,
      std::string_view url,
      RequestOptions options = {});

  [[nodiscard]] vix::async::core::task<Response> async_head(
      vix::async::core::io_context &ctx,
      std::string_view url,
      RequestOptions options = {});

} // namespace vix::requests

#endif // VIX_REQUESTS_CLIENT_HPP

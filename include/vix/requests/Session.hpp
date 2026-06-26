/**
 *
 *  @file Session.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Reusable HTTP session for the Vix requests module.
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

#ifndef VIX_REQUESTS_SESSION_HPP
#define VIX_REQUESTS_SESSION_HPP

#include <vix/requests/Body.hpp>
#include <vix/requests/Client.hpp>
#include <vix/requests/Headers.hpp>
#include <vix/requests/Method.hpp>
#include <vix/requests/Params.hpp>
#include <vix/requests/Request.hpp>
#include <vix/requests/RequestOptions.hpp>
#include <vix/requests/Response.hpp>
#include <vix/async/core/task.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace vix::async::core
{
  class io_context;
}

namespace vix::requests
{
  /**
   * @brief Internal session runtime.
   */
  class SessionRuntime;

  /**
   * @brief Reusable HTTP session.
   *
   * Session stores default options and an in-memory cookie jar. It is useful
   * when multiple requests must share headers, query params, timeout settings,
   * authentication, and cookies.
   */
  class Session
  {
  public:
    /**
     * @brief Creates a session with default options.
     */
    Session();

    /**
     * @brief Creates a session with custom default options.
     *
     * @param defaults Default request options.
     */
    explicit Session(RequestOptions defaults);

    /**
     * @brief Destroys the session.
     */
    ~Session();

    /**
     * @brief Session cannot be copied.
     */
    Session(const Session &) = delete;

    /**
     * @brief Session cannot be copied.
     */
    Session &operator=(const Session &) = delete;

    /**
     * @brief Moves a session.
     */
    Session(Session &&other) noexcept;

    /**
     * @brief Moves a session.
     */
    Session &operator=(Session &&other) noexcept;

    /**
     * @brief Returns mutable default options.
     *
     * @return Default options.
     */
    [[nodiscard]] RequestOptions &defaults() noexcept;

    /**
     * @brief Returns read-only default options.
     *
     * @return Default options.
     */
    [[nodiscard]] const RequestOptions &defaults() const noexcept;

    /**
     * @brief Replaces default options.
     *
     * @param options New default options.
     */
    void set_defaults(RequestOptions options);

    /**
     * @brief Returns mutable default headers.
     *
     * @return Default headers.
     */
    [[nodiscard]] Headers &headers() noexcept;

    /**
     * @brief Returns read-only default headers.
     *
     * @return Default headers.
     */
    [[nodiscard]] const Headers &headers() const noexcept;

    /**
     * @brief Returns mutable default query params.
     *
     * @return Default query params.
     */
    [[nodiscard]] Params &params() noexcept;

    /**
     * @brief Returns read-only default query params.
     *
     * @return Default query params.
     */
    [[nodiscard]] const Params &params() const noexcept;

    /**
     * @brief Returns mutable default timeout.
     *
     * @return Default timeout.
     */
    [[nodiscard]] Timeout &timeout() noexcept;

    /**
     * @brief Returns read-only default timeout.
     *
     * @return Default timeout.
     */
    [[nodiscard]] const Timeout &timeout() const noexcept;

    /**
     * @brief Sets a default header.
     *
     * @param name Header name.
     * @param value Header value.
     * @return This session.
     */
    Session &set_header(
        std::string_view name,
        std::string_view value);

    /**
     * @brief Removes a default header.
     *
     * @param name Header name.
     * @return This session.
     */
    Session &remove_header(std::string_view name);

    /**
     * @brief Sets a default query parameter.
     *
     * @param name Parameter name.
     * @param value Parameter value.
     * @return This session.
     */
    Session &set_param(
        std::string_view name,
        std::string_view value);

    /**
     * @brief Removes a default query parameter.
     *
     * @param name Parameter name.
     * @return This session.
     */
    Session &remove_param(std::string_view name);

    /**
     * @brief Sets default basic authentication.
     *
     * @param username Username.
     * @param password Password.
     * @return This session.
     */
    Session &set_basic_auth(
        std::string username,
        std::string password);

    /**
     * @brief Clears stored cookies.
     *
     * @return This session.
     */
    Session &clear_cookies();

    /**
     * @brief Sends a prepared request.
     *
     * @param request Prepared request.
     * @return HTTP response.
     */
    [[nodiscard]] Response send(const Request &request);

    /**
     * @brief Asynchronously sends a prepared request.
     *
     * @param ctx Async runtime context.
     * @param request Prepared request.
     * @return Task producing the HTTP response.
     */
    [[nodiscard]] vix::async::core::task<Response> async_send(
        vix::async::core::io_context &ctx,
        const Request &request);

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
        Body body = {});

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
        Body body = {});

    /**
     * @brief Sends a GET request.
     *
     * @param url Target URL.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response get(
        std::string_view url,
        RequestOptions options = {});

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
        RequestOptions options = {});

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
        RequestOptions options = {});

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
        RequestOptions options = {});

    /**
     * @brief Sends a DELETE request.
     *
     * @param url Target URL.
     * @param options Request options.
     * @return HTTP response.
     */
    [[nodiscard]] Response del(
        std::string_view url,
        RequestOptions options = {});

    /**
     * @brief Sends a HEAD request.
     *
     * @param url Target URL.
     * @param options Request options.
     * @return HTTP response.
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

  private:
    /**
     * @brief Runtime state.
     */
    std::unique_ptr<SessionRuntime> runtime_;
  };

} // namespace vix::requests

#endif // VIX_REQUESTS_SESSION_HPP

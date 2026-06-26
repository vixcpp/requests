/**
 *
 *  @file HttpsTransport.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTPS transport for the Vix requests module.
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

#ifndef VIX_REQUESTS_TRANSPORT_HTTPS_TRANSPORT_HPP
#define VIX_REQUESTS_TRANSPORT_HTTPS_TRANSPORT_HPP

#include <vix/requests/transport/Transport.hpp>
#include <vix/async/core/task.hpp>

#include <cstddef>
#include <string>

namespace vix::async::core
{
  class io_context;
}

namespace vix::requests::transport
{
  /**
   * @brief HTTPS transport backed by Asio and OpenSSL.
   */
  class HttpsTransport final : public Transport
  {
  public:
    /**
     * @brief Creates an HTTPS transport.
     */
    HttpsTransport() = default;

    /**
     * @brief Sends one HTTPS request synchronously.
     *
     * @param request Prepared request.
     * @return HTTP response.
     */
    [[nodiscard]] Response send(const Request &request) override;

    /**
     * @brief Sends one HTTPS request asynchronously.
     *
     * @param ctx Async runtime context.
     * @param request Prepared request.
     * @return Task producing the HTTP response.
     */
    [[nodiscard]] vix::async::core::task<Response> async_send(
        vix::async::core::io_context &ctx,
        const Request &request) override;

    /**
     * @brief Checks whether this transport supports a URL.
     *
     * @param url Parsed URL.
     * @return True for https URLs.
     */
    [[nodiscard]] bool supports(const Url &url) const noexcept override;

    /**
     * @brief Returns the transport protocol.
     *
     * @return HTTPS protocol.
     */
    [[nodiscard]] TransportProtocol protocol() const noexcept override;

    /**
     * @brief Checks whether enough bytes have been read for a full response.
     *
     * @param data Raw response data.
     * @param expectBody Whether a response body is expected.
     * @return True when the response is complete.
     */
    [[nodiscard]] bool response_complete(
        const std::string &data,
        bool expectBody) const;

  private:
    /**
     * @brief Default TLS stream read size.
     */
    static constexpr std::size_t readChunkSize = 16U * 1024U;
  };

} // namespace vix::requests::transport

#endif // VIX_REQUESTS_TRANSPORT_HTTPS_TRANSPORT_HPP

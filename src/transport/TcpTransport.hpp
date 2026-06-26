/**
 *
 *  @file TcpTransport.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Plain HTTP TCP transport for the Vix requests module.
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

#ifndef VIX_REQUESTS_TRANSPORT_TCP_TRANSPORT_HPP
#define VIX_REQUESTS_TRANSPORT_TCP_TRANSPORT_HPP

#include <vix/requests/transport/Transport.hpp>

#include <vix/async/core/task.hpp>

#include <cstddef>
#include <string>

namespace vix::async::core
{
  class io_context;
}

namespace vix::async::net
{
  class tcp_stream;
}

namespace vix::requests::transport
{
  /**
   * @brief Plain HTTP transport backed by TCP sockets.
   *
   * TcpTransport only supports the http scheme. HTTPS is intentionally handled
   * by another future transport backend.
   */
  class TcpTransport final : public Transport
  {
  public:
    /**
     * @brief Creates a TCP transport.
     */
    TcpTransport() = default;

    /**
     * @brief Sends one HTTP request through a TCP socket.
     *
     * @param request Prepared request.
     * @return HTTP response.
     */
    [[nodiscard]] Response send(const Request &request) override;

    /**
     * @brief Asynchronously sends one HTTP request through a TCP stream.
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
     * @return True for http URLs.
     */
    [[nodiscard]] bool supports(const Url &url) const noexcept override;

    /**
     * @brief Returns the transport protocol.
     *
     * @return HTTP protocol.
     */
    [[nodiscard]] TransportProtocol protocol() const noexcept override;

  private:
    /**
     * @brief Default socket read size.
     */
    static constexpr std::size_t readChunkSize = 16U * 1024U;

    /**
     * @brief Opens a socket connected to the request URL.
     *
     * @param url Target URL.
     * @param timeout Timeout configuration.
     * @return Connected socket.
     */
    [[nodiscard]] vix::async::core::task<void> connect(
        vix::async::core::io_context &ctx,
        vix::async::net::tcp_stream &stream,
        const Url &url,
        const Timeout &timeout) const;

    /**
     * @brief Reads one HTTP response from a socket.
     *
     * @param socket Connected socket.
     * @param request Request used to know body expectations.
     * @return Raw response bytes.
     */
    [[nodiscard]] vix::async::core::task<std::string> read_response_bytes(
        vix::async::core::io_context &ctx,
        vix::async::net::tcp_stream &stream,
        const Request &request) const;

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
  };

} // namespace vix::requests::transport

#endif // VIX_REQUESTS_TRANSPORT_TCP_TRANSPORT_HPP

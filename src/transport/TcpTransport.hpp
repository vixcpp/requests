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

#include "transport/Socket.hpp"

#include <cstddef>
#include <string>

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
    [[nodiscard]] Socket connect(
        const Url &url,
        const Timeout &timeout) const;

    /**
     * @brief Reads one HTTP response from a socket.
     *
     * @param socket Connected socket.
     * @param request Request used to know body expectations.
     * @return Raw response bytes.
     */
    [[nodiscard]] std::string read_response_bytes(
        Socket &socket,
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

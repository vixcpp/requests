/**
 *
 *  @file Transport.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Transport abstraction for the Vix requests module.
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

#ifndef VIX_REQUESTS_TRANSPORT_TRANSPORT_HPP
#define VIX_REQUESTS_TRANSPORT_TRANSPORT_HPP

#include <vix/requests/Request.hpp>
#include <vix/requests/Response.hpp>
#include <vix/requests/Url.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace vix::requests::transport
{
  /**
   * @brief Transport protocol handled by a transport backend.
   */
  enum class TransportProtocol
  {
    Http,
    Https
  };

  /**
   * @brief Converts a transport protocol to its URL scheme.
   *
   * @param protocol Transport protocol.
   * @return Scheme string.
   */
  [[nodiscard]] std::string_view to_string(
      TransportProtocol protocol) noexcept;

  /**
   * @brief Converts a URL scheme to a transport protocol.
   *
   * @param scheme URL scheme.
   * @return Transport protocol.
   */
  [[nodiscard]] TransportProtocol protocol_from_scheme(
      std::string_view scheme);

  /**
   * @brief Low-level HTTP transport interface.
   *
   * A transport sends exactly one prepared request and returns exactly one
   * response. Redirects, cookies, sessions, retries, and high-level behavior
   * stay outside the transport layer.
   */
  class Transport
  {
  public:
    /**
     * @brief Destroys the transport.
     */
    virtual ~Transport() = default;

    /**
     * @brief Transport objects cannot be copied through the interface.
     */
    Transport(const Transport &) = delete;

    /**
     * @brief Transport objects cannot be copied through the interface.
     */
    Transport &operator=(const Transport &) = delete;

    /**
     * @brief Moves a transport interface.
     */
    Transport(Transport &&) noexcept = default;

    /**
     * @brief Moves a transport interface.
     */
    Transport &operator=(Transport &&) noexcept = default;

    /**
     * @brief Sends one prepared request.
     *
     * @param request Prepared request.
     * @return HTTP response.
     */
    [[nodiscard]] virtual Response send(const Request &request) = 0;

    /**
     * @brief Returns true when this transport supports a URL.
     *
     * @param url Parsed URL.
     * @return True when supported.
     */
    [[nodiscard]] virtual bool supports(const Url &url) const noexcept = 0;

    /**
     * @brief Returns the transport protocol.
     *
     * @return Transport protocol.
     */
    [[nodiscard]] virtual TransportProtocol protocol() const noexcept = 0;

  protected:
    /**
     * @brief Creates a transport.
     */
    Transport() = default;
  };

  /**
   * @brief Shared transport pointer.
   */
  using TransportPtr = std::unique_ptr<Transport>;

} // namespace vix::requests::transport

#endif // VIX_REQUESTS_TRANSPORT_TRANSPORT_HPP

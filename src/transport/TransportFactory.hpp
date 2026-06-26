/**
 *
 *  @file TransportFactory.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Transport factory for the Vix requests module.
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

#ifndef VIX_REQUESTS_TRANSPORT_TRANSPORT_FACTORY_HPP
#define VIX_REQUESTS_TRANSPORT_TRANSPORT_FACTORY_HPP

#include <vix/requests/Url.hpp>
#include <vix/requests/transport/Transport.hpp>

#include <memory>
#include <string_view>

namespace vix::requests::transport
{
  /**
   * @brief Creates a transport for a protocol.
   *
   * @param protocol Transport protocol.
   * @return Transport instance.
   */
  [[nodiscard]] TransportPtr make_transport(
      TransportProtocol protocol);

  /**
   * @brief Creates a transport for a parsed URL.
   *
   * @param url Parsed URL.
   * @return Transport instance.
   */
  [[nodiscard]] TransportPtr make_transport_for_url(
      const Url &url);

  /**
   * @brief Checks whether a URL scheme is supported by the current build.
   *
   * @param scheme URL scheme.
   * @return True when supported.
   */
  [[nodiscard]] bool scheme_supported(
      std::string_view scheme) noexcept;

  /**
   * @brief Checks whether a URL is supported by the current build.
   *
   * @param url Parsed URL.
   * @return True when supported.
   */
  [[nodiscard]] bool url_supported(const Url &url) noexcept;

} // namespace vix::requests::transport

#endif // VIX_REQUESTS_TRANSPORT_TRANSPORT_FACTORY_HPP

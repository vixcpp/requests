/**
 *
 *  @file TransportFactory.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Transport factory implementation for the Vix requests module.
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

#include "transport/TransportFactory.hpp"
#include <vix/requests/Error.hpp>

#include "detail/CaseInsensitive.hpp"
#include "transport/TcpTransport.hpp"

#include <memory>
#include <sstream>

namespace vix::requests::transport
{
  namespace
  {
    [[nodiscard]] std::string unsupported_protocol_message(
        std::string_view scheme)
    {
      std::ostringstream oss;

      oss << "unsupported URL scheme";

      if (!scheme.empty())
      {
        oss << ": " << scheme;
      }

      if (detail::ascii_iequals(scheme, "https"))
      {
        oss << " (HTTPS transport is not available in this build)";
      }

      return oss.str();
    }
  } // namespace

  std::string_view to_string(
      TransportProtocol protocol) noexcept
  {
    switch (protocol)
    {
    case TransportProtocol::Http:
      return "http";

    case TransportProtocol::Https:
      return "https";
    }

    return "http";
  }

  TransportProtocol protocol_from_scheme(
      std::string_view scheme)
  {
    if (detail::ascii_iequals(scheme, "http"))
    {
      return TransportProtocol::Http;
    }

    if (detail::ascii_iequals(scheme, "https"))
    {
      return TransportProtocol::Https;
    }

    throw UnsupportedProtocolException(
        unsupported_protocol_message(scheme));
  }

  TransportPtr make_transport(
      TransportProtocol protocol)
  {
    switch (protocol)
    {
    case TransportProtocol::Http:
      return std::make_unique<TcpTransport>();

    case TransportProtocol::Https:
      throw UnsupportedProtocolException(
          "HTTPS transport is not available in this build");
    }

    throw UnsupportedProtocolException("unsupported transport protocol");
  }

  TransportPtr make_transport_for_url(
      const Url &url)
  {
    return make_transport(protocol_from_scheme(url.scheme()));
  }

  bool scheme_supported(
      std::string_view scheme) noexcept
  {
    return detail::ascii_iequals(scheme, "http");
  }

  bool url_supported(const Url &url) noexcept
  {
    return scheme_supported(url.scheme());
  }

} // namespace vix::requests::transport

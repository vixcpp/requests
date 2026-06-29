/**
 *
 *  @file Resolver.cpp
 *  @author Gaspard Kirira
 *
 *  @brief DNS resolver implementation for the Vix requests module.
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

#include "transport/Resolver.hpp"
#include <vix/requests/Error.hpp>

#include <cstring>
#include <memory>
#include <sstream>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

namespace vix::requests::transport
{
  namespace
  {
#if defined(_WIN32)
    void ensure_winsock_started()
    {
      struct WinsockSession
      {
        WinsockSession()
        {
          WSADATA data{};
          const int rc = ::WSAStartup(MAKEWORD(2, 2), &data);
          if (rc != 0)
          {
            throw TransportException("failed to initialize Winsock");
          }
        }

        ~WinsockSession()
        {
          ::WSACleanup();
        }
      };

      static WinsockSession session;
      static_cast<void>(session);
    }
#endif

    struct AddrInfoDeleter
    {
      void operator()(addrinfo *info) const noexcept
      {
        if (info != nullptr)
        {
          ::freeaddrinfo(info);
        }
      }
    };

    using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;

    [[nodiscard]] std::string make_resolve_error(
        std::string_view host,
        std::uint16_t port,
        int errorCode)
    {
      std::ostringstream oss;
      oss << "failed to resolve "
          << host
          << ':'
          << port
          << ": "
#if defined(_WIN32)
          << ::gai_strerrorA(errorCode);
#else
          << ::gai_strerror(errorCode);
#endif

      return oss.str();
    }

    [[nodiscard]] std::string port_to_string(std::uint16_t port)
    {
      return std::to_string(static_cast<unsigned int>(port));
    }

    [[nodiscard]] ResolvedAddress make_resolved_address(
        const addrinfo &info)
    {
      ResolvedAddress resolved;
      resolved.family = info.ai_family;
      resolved.socketType = info.ai_socktype;
      resolved.protocol = info.ai_protocol;
      resolved.addressLength = static_cast<SocketAddressLength>(info.ai_addrlen);

      if (info.ai_addr == nullptr ||
          info.ai_addrlen > sizeof(resolved.address))
      {
        throw TransportException("invalid resolved socket address");
      }

      std::memcpy(
          &resolved.address,
          info.ai_addr,
          info.ai_addrlen);

      resolved.ip = socket_address_to_ip(
          reinterpret_cast<const sockaddr *>(&resolved.address),
          resolved.addressLength);

      return resolved;
    }
  } // namespace

  ResolveResult::ResolveResult(Container addresses)
      : addresses_(std::move(addresses))
  {
  }

  bool ResolveResult::empty() const noexcept
  {
    return addresses_.empty();
  }

  std::size_t ResolveResult::size() const noexcept
  {
    return addresses_.size();
  }

  const ResolveResult::Container &ResolveResult::addresses() const noexcept
  {
    return addresses_;
  }

  const ResolvedAddress &ResolveResult::front() const
  {
    if (addresses_.empty())
    {
      throw TransportException("no resolved addresses available");
    }

    return addresses_.front();
  }

  ResolveResult::iterator ResolveResult::begin() noexcept
  {
    return addresses_.begin();
  }

  ResolveResult::iterator ResolveResult::end() noexcept
  {
    return addresses_.end();
  }

  ResolveResult::const_iterator ResolveResult::begin() const noexcept
  {
    return addresses_.begin();
  }

  ResolveResult::const_iterator ResolveResult::end() const noexcept
  {
    return addresses_.end();
  }

  ResolveResult::const_iterator ResolveResult::cbegin() const noexcept
  {
    return addresses_.cbegin();
  }

  ResolveResult::const_iterator ResolveResult::cend() const noexcept
  {
    return addresses_.cend();
  }

  ResolveResult resolve_tcp(
      std::string_view host,
      std::uint16_t port)
  {
#if defined(_WIN32)
    ensure_winsock_started();
#endif

    if (host.empty())
    {
      throw TransportException("cannot resolve empty host");
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_ADDRCONFIG;

    addrinfo *rawResult = nullptr;

    const std::string hostString(host);
    const std::string service = port_to_string(port);

    const int result = ::getaddrinfo(
        hostString.c_str(),
        service.c_str(),
        &hints,
        &rawResult);

    AddrInfoPtr resolvedList(rawResult);

    if (result != 0)
    {
      throw ConnectionException(
          make_resolve_error(host, port, result));
    }

    std::vector<ResolvedAddress> addresses;

    for (const addrinfo *info = resolvedList.get();
         info != nullptr;
         info = info->ai_next)
    {
      if (info->ai_addr == nullptr)
      {
        continue;
      }

      if (info->ai_family != AF_INET &&
          info->ai_family != AF_INET6)
      {
        continue;
      }

      addresses.push_back(make_resolved_address(*info));
    }

    if (addresses.empty())
    {
      throw ConnectionException(
          make_resolve_error(host, port, EAI_NONAME));
    }

    return ResolveResult(std::move(addresses));
  }

  std::string socket_address_to_ip(
      const sockaddr *address,
      SocketAddressLength addressLength)
  {
    if (address == nullptr || addressLength == 0)
    {
      return {};
    }

    char buffer[INET6_ADDRSTRLEN] = {};

    if (address->sa_family == AF_INET)
    {
      const auto *addr4 =
          reinterpret_cast<const sockaddr_in *>(address);

      const char *result = ::inet_ntop(
          AF_INET,
          &addr4->sin_addr,
          buffer,
          sizeof(buffer));

      return result == nullptr ? std::string{} : std::string(result);
    }

    if (address->sa_family == AF_INET6)
    {
      const auto *addr6 =
          reinterpret_cast<const sockaddr_in6 *>(address);

      const char *result = ::inet_ntop(
          AF_INET6,
          &addr6->sin6_addr,
          buffer,
          sizeof(buffer));

      return result == nullptr ? std::string{} : std::string(result);
    }

    return {};
  }

} // namespace vix::requests::transport

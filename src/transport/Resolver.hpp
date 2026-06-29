/**
 *
 *  @file Resolver.hpp
 *  @author Gaspard Kirira
 *
 *  @brief DNS resolver for the Vix requests module.
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

#ifndef VIX_REQUESTS_TRANSPORT_RESOLVER_HPP
#define VIX_REQUESTS_TRANSPORT_RESOLVER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

namespace vix::requests::transport
{
#if defined(_WIN32)
  using SocketAddressLength = int;
#else
  using SocketAddressLength = socklen_t;
#endif

  /**
   * @brief Resolved socket address.
   */
  struct ResolvedAddress
  {
    /**
     * @brief Address family, for example AF_INET or AF_INET6.
     */
    int family = 0;

    /**
     * @brief Socket type, usually SOCK_STREAM.
     */
    int socketType = 0;

    /**
     * @brief Protocol, usually IPPROTO_TCP.
     */
    int protocol = 0;

    /**
     * @brief Raw socket address bytes.
     */
    sockaddr_storage address{};

    /**
     * @brief Raw socket address size.
     */
    SocketAddressLength addressLength = 0;

    /**
     * @brief Human-readable resolved IP address.
     */
    std::string ip;
  };

  /**
   * @brief DNS resolver result.
   */
  class ResolveResult
  {
  public:
    using Container = std::vector<ResolvedAddress>;
    using iterator = Container::iterator;
    using const_iterator = Container::const_iterator;

    /**
     * @brief Creates an empty result.
     */
    ResolveResult() = default;

    /**
     * @brief Creates a result from resolved addresses.
     *
     * @param addresses Resolved addresses.
     */
    explicit ResolveResult(Container addresses);

    /**
     * @brief Checks whether no address was resolved.
     *
     * @return True when empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns resolved address count.
     *
     * @return Address count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns all resolved addresses.
     *
     * @return Addresses.
     */
    [[nodiscard]] const Container &addresses() const noexcept;

    /**
     * @brief Returns the first resolved address.
     *
     * @return First address.
     */
    [[nodiscard]] const ResolvedAddress &front() const;

    /**
     * @brief Returns a mutable iterator to the first address.
     */
    [[nodiscard]] iterator begin() noexcept;

    /**
     * @brief Returns a mutable iterator past the last address.
     */
    [[nodiscard]] iterator end() noexcept;

    /**
     * @brief Returns a read-only iterator to the first address.
     */
    [[nodiscard]] const_iterator begin() const noexcept;

    /**
     * @brief Returns a read-only iterator past the last address.
     */
    [[nodiscard]] const_iterator end() const noexcept;

    /**
     * @brief Returns a read-only iterator to the first address.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept;

    /**
     * @brief Returns a read-only iterator past the last address.
     */
    [[nodiscard]] const_iterator cend() const noexcept;

  private:
    /**
     * @brief Resolved addresses.
     */
    Container addresses_;
  };

  /**
   * @brief Resolves a host and port for TCP.
   *
   * @param host Host name or IP address.
   * @param port TCP port.
   * @return Resolved addresses.
   */
  [[nodiscard]] ResolveResult resolve_tcp(
      std::string_view host,
      std::uint16_t port);

  /**
   * @brief Converts a socket address to a readable IP address.
   *
   * @param address Socket address.
   * @param addressLength Socket address length.
   * @return IP address string.
   */
  [[nodiscard]] std::string socket_address_to_ip(
      const sockaddr *address,
      SocketAddressLength addressLength);

} // namespace vix::requests::transport

#endif // VIX_REQUESTS_TRANSPORT_RESOLVER_HPP

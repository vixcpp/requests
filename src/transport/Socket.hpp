/**
 *
 *  @file Socket.hpp
 *  @author Gaspard Kirira
 *
 *  @brief RAII socket wrapper for the Vix requests module.
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

#ifndef VIX_REQUESTS_TRANSPORT_SOCKET_HPP
#define VIX_REQUESTS_TRANSPORT_SOCKET_HPP

#include <vix/requests/Timeout.hpp>

#include <cstddef>
#include <string>
#include <string_view>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#endif

namespace vix::requests::transport
{
  /**
   * @brief Native socket handle type.
   */
#if defined(_WIN32)
  using NativeSocketHandle = SOCKET;
#else
  using NativeSocketHandle = int;
#endif

  /**
   * @brief Invalid native socket handle.
   */
#if defined(_WIN32)
  inline constexpr NativeSocketHandle invalidSocketHandle = INVALID_SOCKET;
#else
  inline constexpr NativeSocketHandle invalidSocketHandle = -1;
#endif

  /**
   * @brief RAII TCP socket wrapper.
   */
  class Socket
  {
  public:
    /**
     * @brief Creates an invalid socket.
     */
    Socket() noexcept = default;

    /**
     * @brief Takes ownership of a native socket handle.
     *
     * @param handle Native socket handle.
     */
    explicit Socket(NativeSocketHandle handle) noexcept;

    /**
     * @brief Closes the socket.
     */
    ~Socket();

    /**
     * @brief Socket cannot be copied.
     */
    Socket(const Socket &) = delete;

    /**
     * @brief Socket cannot be copied.
     */
    Socket &operator=(const Socket &) = delete;

    /**
     * @brief Moves a socket.
     *
     * @param other Other socket.
     */
    Socket(Socket &&other) noexcept;

    /**
     * @brief Moves a socket.
     *
     * @param other Other socket.
     * @return This socket.
     */
    Socket &operator=(Socket &&other) noexcept;

    /**
     * @brief Creates a TCP socket for an address family.
     *
     * @param family Address family, for example AF_INET or AF_INET6.
     * @return Socket.
     */
    [[nodiscard]] static Socket tcp(int family);

    /**
     * @brief Connects to a resolved socket address.
     *
     * @param address Pointer to sockaddr-compatible data.
     * @param addressLength Address length.
     * @param timeout Timeout configuration.
     */
    void connect(
        const void *address,
        std::size_t addressLength,
        const Timeout &timeout);

    /**
     * @brief Sends all bytes.
     *
     * @param data Data to send.
     * @param timeout Timeout configuration.
     * @return Number of bytes sent.
     */
    std::size_t send_all(
        std::string_view data,
        const Timeout &timeout);

    /**
     * @brief Receives up to maxBytes bytes.
     *
     * @param maxBytes Maximum bytes to read.
     * @param timeout Timeout configuration.
     * @return Received bytes. Empty string can mean EOF.
     */
    [[nodiscard]] std::string receive(
        std::size_t maxBytes,
        const Timeout &timeout);

    /**
     * @brief Closes the socket.
     */
    void close() noexcept;

    /**
     * @brief Releases ownership of the native handle.
     *
     * @return Native socket handle.
     */
    [[nodiscard]] NativeSocketHandle release() noexcept;

    /**
     * @brief Returns the native socket handle.
     *
     * @return Native socket handle.
     */
    [[nodiscard]] NativeSocketHandle native_handle() const noexcept;

    /**
     * @brief Checks whether the socket owns a valid handle.
     *
     * @return True when valid.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Checks whether the socket owns a valid handle.
     *
     * @return True when valid.
     */
    explicit operator bool() const noexcept;

  private:
    /**
     * @brief Native socket handle.
     */
    NativeSocketHandle handle_{invalidSocketHandle};

    /**
     * @brief Sets blocking or non-blocking mode.
     *
     * @param enabled True for non-blocking mode.
     */
    void set_non_blocking(bool enabled);

    /**
     * @brief Enables close-on-exec when supported.
     */
    void set_close_on_exec();

    /**
     * @brief Waits until socket is readable.
     *
     * @param timeout Timeout value.
     */
    void wait_readable(Timeout::Duration timeout) const;

    /**
     * @brief Waits until socket is writable.
     *
     * @param timeout Timeout value.
     */
    void wait_writable(Timeout::Duration timeout) const;
  };

} // namespace vix::requests::transport

#endif // VIX_REQUESTS_TRANSPORT_SOCKET_HPP

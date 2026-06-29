/**
 *
 *  @file Socket.cpp
 *  @author Gaspard Kirira
 *
 *  @brief RAII socket wrapper implementation for the Vix requests module.
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

#include "transport/Socket.hpp"
#include <vix/requests/Error.hpp>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <sstream>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace vix::requests::transport
{
  namespace
  {
#if defined(_WIN32)
    using NativeSocketAddressLength = int;
#else
    using NativeSocketAddressLength = socklen_t;
#endif

    [[nodiscard]] std::string socket_error_message(
        std::string_view prefix,
        int errorCode)
    {
      std::ostringstream oss;
      oss << prefix << ": ";
#if defined(_WIN32)
      oss << std::system_category().message(errorCode);
#else
      oss << std::strerror(errorCode);
#endif
      return oss.str();
    }

    [[nodiscard]] int last_socket_error() noexcept
    {
#if defined(_WIN32)
      return ::WSAGetLastError();
#else
      return errno;
#endif
    }

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

    [[nodiscard]] timeval to_timeval(Timeout::Duration duration)
    {
      timeval tv{};
      tv.tv_sec = static_cast<long>(duration.count() / 1000);
      tv.tv_usec = static_cast<long>((duration.count() % 1000) * 1000);
      return tv;
    }

    [[nodiscard]] bool would_block(int errorCode) noexcept
    {
#if defined(_WIN32)
      return errorCode == WSAEWOULDBLOCK;
#else
      return errorCode == EAGAIN || errorCode == EWOULDBLOCK;
#endif
    }

    [[nodiscard]] bool interrupted(int errorCode) noexcept
    {
#if defined(_WIN32)
      return errorCode == WSAEINTR;
#else
      return errorCode == EINTR;
#endif
    }

    [[nodiscard]] bool connect_in_progress(int errorCode) noexcept
    {
#if defined(_WIN32)
      return errorCode == WSAEINPROGRESS ||
             errorCode == WSAEALREADY ||
             would_block(errorCode);
#else
      return errorCode == EINPROGRESS || would_block(errorCode);
#endif
    }

    void validate_address_length(std::size_t addressLength)
    {
#if defined(_WIN32)
      constexpr auto maxSocketAddressLength =
          static_cast<std::size_t>(std::numeric_limits<int>::max());
#else
      constexpr auto maxSocketAddressLength =
          static_cast<std::size_t>(std::numeric_limits<socklen_t>::max());
#endif

      if (addressLength > maxSocketAddressLength)
      {
        throw TransportException("socket address is too large");
      }
    }
  } // namespace

  Socket::Socket(NativeSocketHandle handle) noexcept
      : handle_(handle)
  {
  }

  Socket::~Socket()
  {
    close();
  }

  Socket::Socket(Socket &&other) noexcept
      : handle_(other.release())
  {
  }

  Socket &Socket::operator=(Socket &&other) noexcept
  {
    if (this != &other)
    {
      close();
      handle_ = other.release();
    }

    return *this;
  }

  Socket Socket::tcp(int family)
  {
#if defined(_WIN32)
    ensure_winsock_started();
#endif

    const NativeSocketHandle handle =
        ::socket(family, SOCK_STREAM, IPPROTO_TCP);

    if (handle == invalidSocketHandle)
    {
      throw TransportException(
          socket_error_message("failed to create socket", last_socket_error()));
    }

    Socket socket(handle);
    socket.set_close_on_exec();
    return socket;
  }

  void Socket::connect(
      const void *address,
      std::size_t addressLength,
      const Timeout &timeout)
  {
    if (!valid())
    {
      throw ConnectionException("cannot connect invalid socket");
    }

    if (address == nullptr)
    {
      throw ConnectionException("cannot connect to null socket address");
    }

    validate_address_length(addressLength);

    const bool useTimeout = timeout.has_connect();

    if (useTimeout)
    {
      set_non_blocking(true);
    }

    const auto *sockaddrPtr =
        static_cast<const sockaddr *>(address);

    const auto length =
        static_cast<NativeSocketAddressLength>(addressLength);

    int result = ::connect(handle_, sockaddrPtr, length);

    if (result == 0)
    {
      if (useTimeout)
      {
        set_non_blocking(false);
      }

      return;
    }

    const int connectError = last_socket_error();

    if (!useTimeout || !connect_in_progress(connectError))
    {
      throw ConnectionException(
          socket_error_message("failed to connect socket", connectError));
    }

    wait_writable(timeout.connect());

    int socketError = 0;
    NativeSocketAddressLength socketErrorLength = sizeof(socketError);

    if (::getsockopt(
            handle_,
            SOL_SOCKET,
            SO_ERROR,
#if defined(_WIN32)
            reinterpret_cast<char *>(&socketError),
#else
            &socketError,
#endif
            &socketErrorLength) != 0)
    {
      throw ConnectionException(
          socket_error_message("failed to inspect socket connection", last_socket_error()));
    }

    if (socketError != 0)
    {
      throw ConnectionException(
          socket_error_message("failed to connect socket", socketError));
    }

    set_non_blocking(false);
  }

  std::size_t Socket::send_all(
      std::string_view data,
      const Timeout &timeout)
  {
    if (!valid())
    {
      throw TransportException("cannot send on invalid socket");
    }

    std::size_t totalSent = 0;

    while (totalSent < data.size())
    {
      if (timeout.has_read())
      {
        wait_writable(timeout.read());
      }

      const char *buffer = data.data() + totalSent;
      const std::size_t remaining = data.size() - totalSent;

      const auto sent = ::send(
          handle_,
          buffer,
#if defined(_WIN32)
          static_cast<int>(std::min<std::size_t>(
              remaining,
              static_cast<std::size_t>(std::numeric_limits<int>::max()))),
          0);
#else
          remaining,
          MSG_NOSIGNAL);
#endif

      if (sent > 0)
      {
        totalSent += static_cast<std::size_t>(sent);
        continue;
      }

      if (sent == 0)
      {
        throw ConnectionException("socket closed while sending");
      }

      const int sendError = last_socket_error();

      if (interrupted(sendError))
      {
        continue;
      }

      if (would_block(sendError))
      {
        wait_writable(timeout.read());
        continue;
      }

      throw TransportException(
          socket_error_message("failed to send socket data", sendError));
    }

    return totalSent;
  }

  std::string Socket::receive(
      std::size_t maxBytes,
      const Timeout &timeout)
  {
    if (!valid())
    {
      throw TransportException("cannot receive from invalid socket");
    }

    if (maxBytes == 0)
    {
      return {};
    }

    if (timeout.has_read())
    {
      wait_readable(timeout.read());
    }

    std::string buffer;
    buffer.resize(maxBytes);

    while (true)
    {
      const auto received = ::recv(
          handle_,
          buffer.data(),
#if defined(_WIN32)
          static_cast<int>(std::min<std::size_t>(
              buffer.size(),
              static_cast<std::size_t>(std::numeric_limits<int>::max()))),
#else
          buffer.size(),
#endif
          0);

      if (received > 0)
      {
        buffer.resize(static_cast<std::size_t>(received));
        return buffer;
      }

      if (received == 0)
      {
        return {};
      }

      const int recvError = last_socket_error();

      if (interrupted(recvError))
      {
        continue;
      }

      if (would_block(recvError))
      {
        wait_readable(timeout.read());
        continue;
      }

      throw TransportException(
          socket_error_message("failed to receive socket data", recvError));
    }
  }

  void Socket::close() noexcept
  {
    if (valid())
    {
#if defined(_WIN32)
      ::closesocket(handle_);
#else
      ::close(handle_);
#endif
      handle_ = invalidSocketHandle;
    }
  }

  NativeSocketHandle Socket::release() noexcept
  {
    const NativeSocketHandle handle = handle_;
    handle_ = invalidSocketHandle;
    return handle;
  }

  NativeSocketHandle Socket::native_handle() const noexcept
  {
    return handle_;
  }

  bool Socket::valid() const noexcept
  {
    return handle_ != invalidSocketHandle;
  }

  Socket::operator bool() const noexcept
  {
    return valid();
  }

  void Socket::set_non_blocking(bool enabled)
  {
    if (!valid())
    {
      throw TransportException("cannot change mode of invalid socket");
    }

#if defined(_WIN32)
    u_long mode = enabled ? 1UL : 0UL;
    if (::ioctlsocket(handle_, FIONBIO, &mode) != 0)
    {
      throw TransportException(
          socket_error_message("failed to update socket mode", last_socket_error()));
    }
#else
    const int flags = ::fcntl(handle_, F_GETFL, 0);
    if (flags == -1)
    {
      throw TransportException(
          socket_error_message("failed to read socket flags", errno));
    }

    const int nextFlags = enabled
                              ? flags | O_NONBLOCK
                              : flags & ~O_NONBLOCK;

    if (::fcntl(handle_, F_SETFL, nextFlags) == -1)
    {
      throw TransportException(
          socket_error_message("failed to update socket flags", errno));
    }
#endif
  }

  void Socket::set_close_on_exec()
  {
    if (!valid())
    {
      return;
    }

#if !defined(_WIN32)
    const int flags = ::fcntl(handle_, F_GETFD, 0);
    if (flags == -1)
    {
      return;
    }

    static_cast<void>(::fcntl(handle_, F_SETFD, flags | FD_CLOEXEC));
#endif
  }

  void Socket::wait_readable(Timeout::Duration timeout) const
  {
    if (!valid())
    {
      throw TransportException("cannot wait on invalid socket");
    }

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(handle_, &readSet);

    timeval tv = to_timeval(timeout);

    const int result = ::select(
#if defined(_WIN32)
        0,
#else
        handle_ + 1,
#endif
        &readSet,
        nullptr,
        nullptr,
        timeout.count() > 0 ? &tv : nullptr);

    if (result > 0)
    {
      return;
    }

    if (result == 0)
    {
      throw TimeoutException("socket read timed out");
    }

    const int waitError = last_socket_error();

    if (interrupted(waitError))
    {
      wait_readable(timeout);
      return;
    }

    throw TransportException(
        socket_error_message("failed waiting for socket read", waitError));
  }

  void Socket::wait_writable(Timeout::Duration timeout) const
  {
    if (!valid())
    {
      throw TransportException("cannot wait on invalid socket");
    }

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(handle_, &writeSet);

    timeval tv = to_timeval(timeout);

    const int result = ::select(
#if defined(_WIN32)
        0,
#else
        handle_ + 1,
#endif
        nullptr,
        &writeSet,
        nullptr,
        timeout.count() > 0 ? &tv : nullptr);

    if (result > 0)
    {
      return;
    }

    if (result == 0)
    {
      throw TimeoutException("socket write timed out");
    }

    const int waitError = last_socket_error();

    if (interrupted(waitError))
    {
      wait_writable(timeout);
      return;
    }

    throw TransportException(
        socket_error_message("failed waiting for socket write", waitError));
  }

} // namespace vix::requests::transport

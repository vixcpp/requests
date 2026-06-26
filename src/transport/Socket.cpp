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

#include <cerrno>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace vix::requests::transport
{
  namespace
  {
    [[nodiscard]] std::string errno_message(
        std::string_view prefix,
        int errorCode)
    {
      std::ostringstream oss;
      oss << prefix << ": " << std::strerror(errorCode);
      return oss.str();
    }

    [[nodiscard]] timeval to_timeval(Timeout::Duration duration)
    {
      timeval tv{};
      tv.tv_sec = static_cast<long>(duration.count() / 1000);
      tv.tv_usec = static_cast<long>((duration.count() % 1000) * 1000);
      return tv;
    }

    [[nodiscard]] bool would_block(int errorCode) noexcept
    {
      return errorCode == EAGAIN || errorCode == EWOULDBLOCK;
    }

    [[nodiscard]] bool interrupted(int errorCode) noexcept
    {
      return errorCode == EINTR;
    }

    void validate_address_length(std::size_t addressLength)
    {
      if (addressLength >
          static_cast<std::size_t>(std::numeric_limits<socklen_t>::max()))
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
    const NativeSocketHandle handle =
        ::socket(family, SOCK_STREAM, IPPROTO_TCP);

    if (handle == invalidSocketHandle)
    {
      throw TransportException(errno_message("failed to create socket", errno));
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
        static_cast<socklen_t>(addressLength);

    int result = ::connect(handle_, sockaddrPtr, length);

    if (result == 0)
    {
      if (useTimeout)
      {
        set_non_blocking(false);
      }

      return;
    }

    const int connectError = errno;

    if (!useTimeout ||
        (connectError != EINPROGRESS && !would_block(connectError)))
    {
      throw ConnectionException(
          errno_message("failed to connect socket", connectError));
    }

    wait_writable(timeout.connect());

    int socketError = 0;
    socklen_t socketErrorLength = sizeof(socketError);

    if (::getsockopt(
            handle_,
            SOL_SOCKET,
            SO_ERROR,
            &socketError,
            &socketErrorLength) != 0)
    {
      throw ConnectionException(
          errno_message("failed to inspect socket connection", errno));
    }

    if (socketError != 0)
    {
      throw ConnectionException(
          errno_message("failed to connect socket", socketError));
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

      const ssize_t sent = ::send(
          handle_,
          buffer,
          remaining,
          MSG_NOSIGNAL);

      if (sent > 0)
      {
        totalSent += static_cast<std::size_t>(sent);
        continue;
      }

      if (sent == 0)
      {
        throw ConnectionException("socket closed while sending");
      }

      const int sendError = errno;

      if (interrupted(sendError))
      {
        continue;
      }

      if (would_block(sendError))
      {
        wait_writable(timeout.read());
        continue;
      }

      throw TransportException(errno_message("failed to send socket data", sendError));
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
      const ssize_t received = ::recv(
          handle_,
          buffer.data(),
          buffer.size(),
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

      const int recvError = errno;

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
          errno_message("failed to receive socket data", recvError));
    }
  }

  void Socket::close() noexcept
  {
    if (valid())
    {
      ::close(handle_);
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

    const int flags = ::fcntl(handle_, F_GETFL, 0);
    if (flags == -1)
    {
      throw TransportException(
          errno_message("failed to read socket flags", errno));
    }

    const int nextFlags = enabled
                              ? flags | O_NONBLOCK
                              : flags & ~O_NONBLOCK;

    if (::fcntl(handle_, F_SETFL, nextFlags) == -1)
    {
      throw TransportException(
          errno_message("failed to update socket flags", errno));
    }
  }

  void Socket::set_close_on_exec()
  {
    if (!valid())
    {
      return;
    }

    const int flags = ::fcntl(handle_, F_GETFD, 0);
    if (flags == -1)
    {
      return;
    }

    static_cast<void>(::fcntl(handle_, F_SETFD, flags | FD_CLOEXEC));
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
        handle_ + 1,
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

    if (interrupted(errno))
    {
      wait_readable(timeout);
      return;
    }

    throw TransportException(errno_message("failed waiting for socket read", errno));
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
        handle_ + 1,
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

    if (interrupted(errno))
    {
      wait_writable(timeout);
      return;
    }

    throw TransportException(errno_message("failed waiting for socket write", errno));
  }

} // namespace vix::requests::transport

/**
 *
 *  @file TcpTransport.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Plain HTTP TCP transport implementation.
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

#include "transport/TcpTransport.hpp"
#include <vix/requests/Error.hpp>
#include <vix/async/core/cancel.hpp>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/net/tcp.hpp>

#include "http/HttpParser.hpp"
#include "http/HttpSerializer.hpp"
#include "detail/CaseInsensitive.hpp"
#include "transport/Resolver.hpp"
#include "transport/Socket.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <exception>
#include <span>
#include <string>
#include <system_error>
#include <utility>

namespace vix::requests::transport
{
  namespace
  {
    namespace core = vix::async::core;
    namespace net = vix::async::net;

    [[nodiscard]] bool is_final_chunked_response(
        std::string_view body) noexcept
    {
      std::size_t cursor = 0;

      while (cursor < body.size())
      {
        const std::size_t lineEnd = body.find("\r\n", cursor);
        if (lineEnd == std::string_view::npos)
        {
          return false;
        }

        const std::string_view sizeLine =
            body.substr(cursor, lineEnd - cursor);

        const std::size_t semicolon = sizeLine.find(';');
        const std::string_view sizeText =
            semicolon == std::string_view::npos
                ? sizeLine
                : sizeLine.substr(0, semicolon);

        std::size_t chunkSize = 0;

        if (sizeText.empty())
        {
          return false;
        }

        for (char raw_ch : sizeText)
        {
          const auto ch = static_cast<unsigned char>(raw_ch);
          unsigned int digit = 0;

          if (ch >= '0' && ch <= '9')
          {
            digit = static_cast<unsigned int>(ch - '0');
          }
          else if (ch >= 'a' && ch <= 'f')
          {
            digit = static_cast<unsigned int>(ch - 'a' + 10);
          }
          else if (ch >= 'A' && ch <= 'F')
          {
            digit = static_cast<unsigned int>(ch - 'A' + 10);
          }
          else
          {
            return false;
          }

          chunkSize = (chunkSize * 16U) + digit;
        }

        cursor = lineEnd + 2U;

        if (chunkSize == 0U)
        {
          return body.find("\r\n\r\n", cursor) != std::string_view::npos ||
                 body.find("\r\n", cursor) != std::string_view::npos;
        }

        if (cursor + chunkSize + 2U > body.size())
        {
          return false;
        }

        cursor += chunkSize;

        if (body.substr(cursor, 2U) != "\r\n")
        {
          return false;
        }

        cursor += 2U;
      }

      return false;
    }

    [[nodiscard]] bool is_cancelled_error(const std::system_error &error)
    {
      return error.code() == core::cancelled_ec();
    }

    [[nodiscard]] bool connection_requests_close(const Headers &headers)
    {
      const auto value = headers.get("Connection");
      if (!value.has_value()) return false;
      std::size_t start = 0;
      while (start < value->size())
      {
        const std::size_t comma = value->find(',', start);
        const std::string_view token = std::string_view(*value).substr(
            start, comma == std::string::npos ? std::string::npos : comma - start);
        std::size_t first = 0;
        while (first < token.size() && (token[first] == ' ' || token[first] == '\t')) ++first;
        std::size_t last = token.size();
        while (last > first && (token[last - 1] == ' ' || token[last - 1] == '\t')) --last;
        if (detail::ascii_iequals(token.substr(first, last - first), "close")) return true;
        if (comma == std::string::npos) break;
        start = comma + 1U;
      }
      return false;
    }

    [[nodiscard]] bool deadline_expired(
        std::chrono::steady_clock::time_point started,
        Timeout::Duration duration)
    {
      return duration.count() > 0 &&
             std::chrono::steady_clock::now() - started >= duration;
    }

    void schedule_timeout(
        core::io_context &ctx,
        net::tcp_stream &stream,
        Timeout::Duration duration,
        core::cancel_source source)
    {
      if (duration.count() <= 0)
      {
        return;
      }

      ctx.timers().after(
          duration,
          [&stream, source]() mutable
          {
            source.request_cancel();
            stream.close();
          },
          source.token());
    }

    [[nodiscard]] core::task<void> async_write_all(
        core::io_context &ctx,
        net::tcp_stream &stream,
        std::string_view data,
        const Timeout &timeout)
    {
      core::cancel_source source;
      const bool useTimeout = timeout.has_read();

      if (useTimeout)
      {
        schedule_timeout(ctx, stream, timeout.read(), source);
      }

      std::size_t sent = 0;

      try
      {
        while (sent < data.size())
        {
          const auto *ptr = reinterpret_cast<const std::byte *>(
              data.data() + sent);
          const std::size_t remaining = data.size() - sent;

          const auto started = std::chrono::steady_clock::now();
          auto writeSomeTask = stream.async_write(
              std::span<const std::byte>(ptr, remaining),
              useTimeout ? source.token() : core::cancel_token{});
          const std::size_t written = co_await std::move(writeSomeTask);

          if (useTimeout && deadline_expired(started, timeout.read()))
          {
            source.request_cancel();
            stream.close();
            throw TimeoutException("request write timed out");
          }

          if (written == 0U)
          {
            throw ConnectionException("socket closed while sending");
          }

          sent += written;
        }
      }
      catch (const std::system_error &error)
      {
        const bool timedOut = useTimeout && source.is_cancelled();
        source.request_cancel();

        if (timedOut || is_cancelled_error(error))
        {
          throw TimeoutException("request write timed out");
        }

        throw TransportException(error.what());
      }

      source.request_cancel();
      co_return;
    }
  } // namespace

  TcpTransport::TcpTransport() = default;
  TcpTransport::~TcpTransport() = default;

  Response TcpTransport::send(const Request &request)
  {
    if (!supports(request.final_url()))
    {
      throw UnsupportedProtocolException(
          "TcpTransport only supports plain HTTP URLs");
    }

    const auto started = std::chrono::steady_clock::now();
    reusable_ = false;
    if (!socket_.valid())
    {
      const ResolveResult addresses = resolve_tcp(
          request.final_url().host(), request.final_url().port());
      std::exception_ptr lastError;
      for (const ResolvedAddress &address : addresses)
      {
        try
        {
          Socket candidate = Socket::tcp(address.family);
          candidate.connect(&address.address, address.addressLength,
                            request.options().timeout);
          socket_ = std::move(candidate);
          break;
        }
        catch (...) { lastError = std::current_exception(); }
      }
      if (!socket_.valid())
      {
        if (lastError) std::rethrow_exception(lastError);
        throw ConnectionException("failed to connect socket");
      }
    }
    try
    {
      const http::SerializedRequest serialized = http::serialize_request(request);
      static_cast<void>(socket_.send_all(serialized.data, request.options().timeout));
      std::string rawResponse;
      bool sawEof = false;
      while (true)
      {
        const std::string chunk = socket_.receive(readChunkSize, request.options().timeout);
        if (chunk.empty()) { sawEof = true; break; }
        rawResponse += chunk;
        if (response_complete(rawResponse, request.expects_response_body())) break;
      }
      if (rawResponse.empty()) throw ConnectionException("empty HTTP response");
      Response response = http::parse_response(rawResponse,
          request.final_url().without_fragment(), request.expects_response_body());
      const http::BodyInfo framing = http::detect_body_info(response.status_code(),
          response.headers(), request.expects_response_body());
      const bool closes = connection_requests_close(response.headers());
      const bool http11 = rawResponse.rfind("HTTP/1.1", 0) == 0;
      reusable_ = http11 && !sawEof && !closes && framing.framing != http::BodyFraming::ConnectionClose &&
                  socket_.valid();
      if (!reusable_) socket_.close();
      const auto finished = std::chrono::steady_clock::now();
      response.set_elapsed(std::chrono::duration_cast<Response::Duration>(finished - started));
      return response;
    }
    catch (...)
    {
      socket_.close();
      reusable_ = false;
      throw;
    }
  }

  core::task<Response> TcpTransport::async_send(
      core::io_context &ctx,
      Request request)
  {
    if (!supports(request.final_url()))
    {
      throw UnsupportedProtocolException(
          "TcpTransport only supports plain HTTP URLs");
    }

    const auto started = std::chrono::steady_clock::now();

    reusable_ = false;
    try
    {
      if (!stream_)
      {
        stream_ = net::make_tcp_stream(ctx);
        asyncContext_ = &ctx;
        auto connectTask = connect(ctx, *stream_, request.final_url(), request.options().timeout);
        co_await std::move(connectTask);
      }
      else if (asyncContext_ != &ctx)
      {
        throw ConnectionException("TCP connection is bound to another io_context");
      }
      const http::SerializedRequest serialized = http::serialize_request(request);
      auto writeTask = async_write_all(ctx, *stream_, serialized.data, request.options().timeout);
      co_await std::move(writeTask);
      auto readTask = read_response_bytes(ctx, *stream_, request);
      std::string rawResponse = co_await std::move(readTask);
      Response response = http::parse_response(rawResponse, request.final_url().without_fragment(), request.expects_response_body());
      const http::BodyInfo framing = http::detect_body_info(response.status_code(), response.headers(), request.expects_response_body());
      reusable_ = framing.framing != http::BodyFraming::ConnectionClose &&
          !connection_requests_close(response.headers());
      if (!reusable_) discard();
      const auto finished = std::chrono::steady_clock::now();
      response.set_elapsed(std::chrono::duration_cast<Response::Duration>(finished - started));
      co_return response;
    }
    catch (...)
    {
      discard();
      throw;
    }
  }

  bool TcpTransport::supports(const Url &url) const noexcept
  {
    return url.is_http();
  }

  TransportProtocol TcpTransport::protocol() const noexcept
  {
    return TransportProtocol::Http;
  }

  bool TcpTransport::reusable() const noexcept { return reusable_; }

  void TcpTransport::discard() noexcept
  {
    reusable_ = false;
    socket_.close();
    if (stream_) stream_->close();
    stream_.reset();
    asyncContext_ = nullptr;
  }

  bool TcpTransport::async_compatible(const core::io_context &ctx) const noexcept
  {
    return reusable_ && stream_ && asyncContext_ == &ctx;
  }

  core::task<void> TcpTransport::connect(
      core::io_context &ctx,
      net::tcp_stream &stream,
      const Url &url,
      const Timeout &timeout) const
  {
    core::cancel_source source;
    const bool useTimeout = timeout.has_connect();

    if (useTimeout)
    {
      schedule_timeout(ctx, stream, timeout.connect(), source);
    }

    try
    {
      const auto started = std::chrono::steady_clock::now();
      auto connectTask = stream.async_connect(
          net::tcp_endpoint{url.host(), url.port()},
          useTimeout ? source.token() : core::cancel_token{});
      co_await std::move(connectTask);

      if (useTimeout && deadline_expired(started, timeout.connect()))
      {
        source.request_cancel();
        stream.close();
        throw TimeoutException("connection timed out");
      }
    }
    catch (const std::system_error &error)
    {
      const bool timedOut = useTimeout && source.is_cancelled();
      source.request_cancel();

      if (timedOut || is_cancelled_error(error))
      {
        throw TimeoutException("connection timed out");
      }

      throw ConnectionException(error.what());
    }

    source.request_cancel();
    co_return;
  }

  core::task<std::string> TcpTransport::read_response_bytes(
      core::io_context &ctx,
      net::tcp_stream &stream,
      const Request &request) const
  {
    std::string data;
    std::array<std::byte, readChunkSize> buffer{};
    core::cancel_source source;
    const bool useTimeout = request.options().timeout.has_read();

    if (useTimeout)
    {
      schedule_timeout(ctx, stream, request.options().timeout.read(), source);
    }

    while (true)
    {
      std::size_t bytes = 0;

      try
      {
        const auto started = std::chrono::steady_clock::now();
        auto readSomeTask = stream.async_read(
            std::span<std::byte>(buffer.data(), buffer.size()),
            useTimeout ? source.token() : core::cancel_token{});
        bytes = co_await std::move(readSomeTask);

        if (useTimeout && deadline_expired(started, request.options().timeout.read()))
        {
          source.request_cancel();
          stream.close();
          throw TimeoutException("request read timed out");
        }
      }
      catch (const std::system_error &error)
      {
        const bool timedOut = useTimeout && source.is_cancelled();
        source.request_cancel();

        if (timedOut || is_cancelled_error(error))
        {
          throw TimeoutException("request read timed out");
        }

        if (!data.empty())
        {
          break;
        }

        throw ConnectionException(error.what());
      }

      if (bytes == 0U)
      {
        break;
      }

      const auto *chars = reinterpret_cast<const char *>(buffer.data());
      data.append(chars, bytes);

      if (response_complete(data, request.expects_response_body()))
      {
        break;
      }
    }

    source.request_cancel();

    if (data.empty())
    {
      throw ConnectionException("empty HTTP response");
    }

    co_return data;
  }

  bool TcpTransport::response_complete(
      const std::string &data,
      bool expectBody) const
  {
    std::size_t offset = 0;

    while (true)
    {
      const auto headerEnd = http::find_header_end(
          std::string_view(data).substr(offset));

      if (!headerEnd.has_value())
      {
        return false;
      }

      const http::ParsedResponseHead head =
          http::parse_response_head(
              std::string_view(data).substr(offset));

      offset += head.headerSize;

      if (head.statusCode >= 100 &&
          head.statusCode < 200 &&
          head.statusCode != 101)
      {
        continue;
      }

      const std::string_view body =
          std::string_view(data).substr(offset);

      const http::BodyInfo bodyInfo = http::detect_body_info(
          head.statusCode,
          head.headers,
          expectBody);

      switch (bodyInfo.framing)
      {
      case http::BodyFraming::None:
        return true;

      case http::BodyFraming::ContentLength:
        return body.size() >= bodyInfo.contentLength;

      case http::BodyFraming::Chunked:
        return is_final_chunked_response(body);

      case http::BodyFraming::ConnectionClose:
        return false;
      }
    }
  }

} // namespace vix::requests::transport

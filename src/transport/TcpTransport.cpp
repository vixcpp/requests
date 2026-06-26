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

        for (unsigned char ch : sizeText)
        {
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

    core::task<void> drive_sync(
        core::io_context &ctx,
        core::task<Response> &task,
        Response &response,
        std::exception_ptr &error)
    {
      try
      {
        response = co_await task;
      }
      catch (...)
      {
        error = std::current_exception();
      }

      ctx.stop();
      co_return;
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
          const std::size_t written = co_await writeSomeTask;

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

  Response TcpTransport::send(const Request &request)
  {
    core::io_context ctx;
    std::exception_ptr error;
    Response response;

    auto pending = async_send(ctx, request);
    auto runner = drive_sync(ctx, pending, response, error);

    ctx.post(runner.handle());
    ctx.run();

    if (error)
    {
      std::rethrow_exception(error);
    }

    return response;
  }

  core::task<Response> TcpTransport::async_send(
      core::io_context &ctx,
      const Request &request)
  {
    if (!supports(request.final_url()))
    {
      throw UnsupportedProtocolException(
          "TcpTransport only supports plain HTTP URLs");
    }

    const auto started = std::chrono::steady_clock::now();

    auto stream = net::make_tcp_stream(ctx);

    auto connectTask = connect(
        ctx,
        *stream,
        request.final_url(),
        request.options().timeout);
    co_await connectTask;

    const http::SerializedRequest serialized =
        http::serialize_request(request);

    auto writeTask = async_write_all(
        ctx,
        *stream,
        serialized.data,
        request.options().timeout);
    co_await writeTask;

    auto readTask = read_response_bytes(
        ctx,
        *stream,
        request);
    std::string rawResponse = co_await readTask;

    stream->close();

    Response response = http::parse_response(
        rawResponse,
        request.final_url().without_fragment(),
        request.expects_response_body());

    const auto finished = std::chrono::steady_clock::now();

    response.set_elapsed(
        std::chrono::duration_cast<Response::Duration>(
            finished - started));

    co_return response;
  }

  bool TcpTransport::supports(const Url &url) const noexcept
  {
    return url.is_http();
  }

  TransportProtocol TcpTransport::protocol() const noexcept
  {
    return TransportProtocol::Http;
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
      co_await connectTask;

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
        bytes = co_await readSomeTask;

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

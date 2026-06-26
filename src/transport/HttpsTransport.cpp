/**
 *
 *  @file HttpsTransport.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTPS transport implementation for the Vix requests module.
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

#include "transport/HttpsTransport.hpp"
#include <vix/requests/Error.hpp>
#include <vix/async/core/cancel.hpp>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/net/asio_net_service.hpp>

#include "http/HttpParser.hpp"
#include "http/HttpSerializer.hpp"

#include <asio/connect.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl.hpp>

#include <openssl/ssl.h>

#include <array>
#include <chrono>
#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace vix::requests::transport
{
  namespace
  {
    namespace core = vix::async::core;
    using tcp = asio::ip::tcp;
    using ssl_stream = asio::ssl::stream<tcp::socket>;

    template <typename T>
    struct asio_result
    {
      std::error_code ec{};
      std::optional<T> value{};
    };

    template <>
    struct asio_result<void>
    {
      std::error_code ec{};
    };

    template <typename Starter, typename T>
    struct asio_awaitable
    {
      core::io_context *ctx{};
      core::cancel_token token{};
      Starter starter;
      asio_result<T> result{};
      std::exception_ptr error{};

      bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(std::coroutine_handle<> handle)
      {
        if (token.is_cancelled())
        {
          ctx->post(handle);
          return;
        }

        try
        {
          if constexpr (std::is_void_v<T>)
          {
            starter(
                [this, handle](std::error_code ec) mutable
                {
                  result.ec = ec;
                  ctx->post(handle);
                });
          }
          else
          {
            starter(
                [this, handle](std::error_code ec, T value) mutable
                {
                  result.ec = ec;

                  if (!ec)
                  {
                    result.value.emplace(std::move(value));
                  }

                  ctx->post(handle);
                });
          }
        }
        catch (...)
        {
          error = std::current_exception();
          ctx->post(handle);
        }
      }

      T await_resume()
      {
        if (token.is_cancelled())
        {
          throw std::system_error(core::cancelled_ec());
        }

        if (error)
        {
          std::rethrow_exception(error);
        }

        if (result.ec)
        {
          throw std::system_error(result.ec);
        }

        if constexpr (std::is_void_v<T>)
        {
          return;
        }
        else
        {
          return std::move(*result.value);
        }
      }
    };

    template <typename T, typename Starter>
    [[nodiscard]] core::task<T> co_asio_value(
        core::io_context &ctx,
        core::cancel_token token,
        Starter starter)
    {
      co_return co_await asio_awaitable<Starter, T>{
          &ctx,
          std::move(token),
          std::move(starter)};
    }

    template <typename Starter>
    [[nodiscard]] core::task<void> co_asio_void(
        core::io_context &ctx,
        core::cancel_token token,
        Starter starter)
    {
      co_await asio_awaitable<Starter, void>{
          &ctx,
          std::move(token),
          std::move(starter)};
    }

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

    void close_stream(ssl_stream &stream) noexcept
    {
      std::error_code ec;
      stream.lowest_layer().cancel(ec);
      ec.clear();
      stream.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
      ec.clear();
      stream.lowest_layer().close(ec);
    }

    void schedule_timeout(
        core::io_context &ctx,
        ssl_stream &stream,
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
            close_stream(stream);
          },
          source.token());
    }

    [[nodiscard]] std::string system_error_message(
        std::string_view prefix,
        const std::system_error &error)
    {
      std::ostringstream oss;
      oss << prefix << ": " << error.what();
      return oss.str();
    }

    core::task<void> drive_sync(
        core::io_context &ctx,
        core::task<Response> task,
        Response &response,
        std::exception_ptr &error)
    {
      try
      {
        response = co_await std::move(task);
      }
      catch (...)
      {
        error = std::current_exception();
      }

      ctx.stop();
      co_return;
    }

    [[nodiscard]] core::task<void> async_connect_tcp(
        core::io_context &ctx,
        ssl_stream &stream,
        const Url &url,
        const Timeout &timeout)
    {
      core::cancel_source source;
      const bool useTimeout = timeout.has_connect();

      if (useTimeout)
      {
        schedule_timeout(ctx, stream, timeout.connect(), source);
      }

      try
      {
        tcp::resolver resolver(ctx.net().asio_ctx());
        const auto started = std::chrono::steady_clock::now();

        auto resolveTask = co_asio_value<tcp::resolver::results_type>(
            ctx,
            useTimeout ? source.token() : core::cancel_token{},
            [&](auto done)
            {
              resolver.async_resolve(
                  url.host(),
                  std::to_string(url.port()),
                  [done = std::move(done)](
                      std::error_code ec,
                      tcp::resolver::results_type results) mutable
                  {
                    done(ec, std::move(results));
                  });
            });
        auto results = co_await std::move(resolveTask);

        auto connectTask = co_asio_void(
            ctx,
            useTimeout ? source.token() : core::cancel_token{},
            [&](auto done)
            {
              asio::async_connect(
                  stream.lowest_layer(),
                  results,
                  [done = std::move(done)](
                      std::error_code ec,
                      const tcp::endpoint &) mutable
                  {
                    done(ec);
                  });
            });
        co_await std::move(connectTask);

        if (useTimeout && deadline_expired(started, timeout.connect()))
        {
          source.request_cancel();
          close_stream(stream);
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

        throw ConnectionException(system_error_message("failed to connect TLS socket", error));
      }

      source.request_cancel();
      co_return;
    }

    void configure_tls(
        asio::ssl::context &tls,
        ssl_stream &stream,
        const Request &request)
    {
      tls.set_options(
          asio::ssl::context::default_workarounds |
          asio::ssl::context::no_sslv2 |
          asio::ssl::context::no_sslv3 |
          asio::ssl::context::no_tlsv1 |
          asio::ssl::context::no_tlsv1_1);

      if (!SSL_set_tlsext_host_name(
              stream.native_handle(),
              request.final_url().host().c_str()))
      {
        throw TransportException("failed to configure TLS SNI");
      }

      if (request.options().verify_tls)
      {
        std::error_code ec;
        tls.set_default_verify_paths(ec);
        if (ec)
        {
          throw TransportException("failed to load default TLS verify paths: " + ec.message());
        }

        stream.set_verify_mode(asio::ssl::verify_peer);
        stream.set_verify_callback(
            asio::ssl::host_name_verification(request.final_url().host()));
      }
      else
      {
        stream.set_verify_mode(asio::ssl::verify_none);
      }
    }

    [[nodiscard]] core::task<void> async_handshake_tls(
        core::io_context &ctx,
        ssl_stream &stream,
        const Timeout &timeout)
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
        auto handshakeTask = co_asio_void(
            ctx,
            useTimeout ? source.token() : core::cancel_token{},
            [&](auto done)
            {
              stream.async_handshake(
                  asio::ssl::stream_base::client,
                  [done = std::move(done)](std::error_code ec) mutable
                  {
                    done(ec);
                  });
            });
        co_await handshakeTask;

        if (useTimeout && deadline_expired(started, timeout.connect()))
        {
          source.request_cancel();
          close_stream(stream);
          throw TimeoutException("TLS handshake timed out");
        }
      }
      catch (const std::system_error &error)
      {
        const bool timedOut = useTimeout && source.is_cancelled();
        source.request_cancel();

        if (timedOut || is_cancelled_error(error))
        {
          throw TimeoutException("TLS handshake timed out");
        }

        throw ConnectionException(system_error_message("TLS handshake failed", error));
      }

      source.request_cancel();
      co_return;
    }

    [[nodiscard]] core::task<void> async_write_all(
        core::io_context &ctx,
        ssl_stream &stream,
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
          const char *ptr = data.data() + sent;
          const std::size_t remaining = data.size() - sent;
          const auto started = std::chrono::steady_clock::now();

          auto writeTask = co_asio_value<std::size_t>(
              ctx,
              useTimeout ? source.token() : core::cancel_token{},
              [&](auto done)
              {
                stream.async_write_some(
                    asio::buffer(ptr, remaining),
                    [done = std::move(done)](
                        std::error_code ec,
                        std::size_t bytes) mutable
                    {
                      done(ec, bytes);
                    });
              });
          const std::size_t written = co_await writeTask;

          if (useTimeout && deadline_expired(started, timeout.read()))
          {
            source.request_cancel();
            close_stream(stream);
            throw TimeoutException("request write timed out");
          }

          if (written == 0U)
          {
            throw ConnectionException("TLS stream closed while sending");
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

        throw TransportException(system_error_message("failed to write TLS data", error));
      }

      source.request_cancel();
      co_return;
    }

    [[nodiscard]] core::task<std::string> async_read_response_bytes(
        const HttpsTransport &transport,
        core::io_context &ctx,
        ssl_stream &stream,
        const Request &request)
    {
      std::string data;
      std::array<char, 16U * 1024U> buffer{};
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
          auto readTask = co_asio_value<std::size_t>(
              ctx,
              useTimeout ? source.token() : core::cancel_token{},
              [&](auto done)
              {
                stream.async_read_some(
                    asio::buffer(buffer.data(), buffer.size()),
                    [done = std::move(done)](
                        std::error_code ec,
                        std::size_t readBytes) mutable
                    {
                      done(ec, readBytes);
                    });
              });
          bytes = co_await readTask;

          if (useTimeout && deadline_expired(started, request.options().timeout.read()))
          {
            source.request_cancel();
            close_stream(stream);
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

          throw ConnectionException(system_error_message("failed to read TLS data", error));
        }

        if (bytes == 0U)
        {
          break;
        }

        data.append(buffer.data(), bytes);

        if (transport.response_complete(data, request.expects_response_body()))
        {
          break;
        }
      }

      source.request_cancel();

      if (data.empty())
      {
        throw ConnectionException("empty HTTPS response");
      }

      co_return data;
    }
  } // namespace

  Response HttpsTransport::send(const Request &request)
  {
    core::io_context ctx;
    std::exception_ptr error;
    Response response;

    auto runner = drive_sync(
        ctx,
        async_send(ctx, request),
        response,
        error);

    ctx.post(runner.handle());
    ctx.run();

    if (error)
    {
      std::rethrow_exception(error);
    }

    return response;
  }

  core::task<Response> HttpsTransport::async_send(
      core::io_context &ctx,
      const Request &request)
  {
    if (!supports(request.final_url()))
    {
      throw UnsupportedProtocolException(
          "HttpsTransport only supports HTTPS URLs");
    }

    const auto started = std::chrono::steady_clock::now();

    asio::ssl::context tls(asio::ssl::context::tls_client);
    ssl_stream stream(ctx.net().asio_ctx(), tls);

    configure_tls(tls, stream, request);

    auto connectTask = async_connect_tcp(
        ctx,
        stream,
        request.final_url(),
        request.options().timeout);
    co_await connectTask;

    auto handshakeTask = async_handshake_tls(
        ctx,
        stream,
        request.options().timeout);
    co_await handshakeTask;

    const http::SerializedRequest serialized =
        http::serialize_request(request);

    auto writeTask = async_write_all(
        ctx,
        stream,
        serialized.data,
        request.options().timeout);
    co_await writeTask;

    auto readTask = async_read_response_bytes(
        *this,
        ctx,
        stream,
        request);
    std::string rawResponse = co_await std::move(readTask);

    close_stream(stream);

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

  bool HttpsTransport::supports(const Url &url) const noexcept
  {
    return url.is_https();
  }

  TransportProtocol HttpsTransport::protocol() const noexcept
  {
    return TransportProtocol::Https;
  }

  bool HttpsTransport::response_complete(
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

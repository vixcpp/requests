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

#include "http/HttpParser.hpp"
#include "http/HttpSerializer.hpp"
#include "transport/Resolver.hpp"

#include <chrono>
#include <exception>
#include <string>
#include <utility>

namespace vix::requests::transport
{
  namespace
  {
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
  } // namespace

  Response TcpTransport::send(const Request &request)
  {
    if (!supports(request.final_url()))
    {
      throw UnsupportedProtocolException(
          "TcpTransport only supports plain HTTP URLs");
    }

    const auto started = std::chrono::steady_clock::now();

    Socket socket = connect(
        request.final_url(),
        request.options().timeout);

    const http::SerializedRequest serialized =
        http::serialize_request(request);

    socket.send_all(
        serialized.data,
        request.options().timeout);

    std::string rawResponse = read_response_bytes(socket, request);

    Response response = http::parse_response(
        rawResponse,
        request.final_url().without_fragment(),
        request.expects_response_body());

    const auto finished = std::chrono::steady_clock::now();

    response.set_elapsed(
        std::chrono::duration_cast<Response::Duration>(
            finished - started));

    return response;
  }

  bool TcpTransport::supports(const Url &url) const noexcept
  {
    return url.is_http();
  }

  TransportProtocol TcpTransport::protocol() const noexcept
  {
    return TransportProtocol::Http;
  }

  Socket TcpTransport::connect(
      const Url &url,
      const Timeout &timeout) const
  {
    const ResolveResult resolved = resolve_tcp(url.host(), url.port());

    std::string lastError;

    for (const ResolvedAddress &address : resolved.addresses())
    {
      try
      {
        Socket socket = Socket::tcp(address.family);

        socket.connect(
            &address.address,
            address.addressLength,
            timeout);

        return socket;
      }
      catch (const RequestException &error)
      {
        lastError = error.what();
      }
    }

    if (!lastError.empty())
    {
      throw ConnectionException(lastError);
    }

    throw ConnectionException("failed to connect socket");
  }

  std::string TcpTransport::read_response_bytes(
      Socket &socket,
      const Request &request) const
  {
    std::string data;

    while (true)
    {
      std::string chunk = socket.receive(
          readChunkSize,
          request.options().timeout);

      if (chunk.empty())
      {
        break;
      }

      data += chunk;

      if (response_complete(data, request.expects_response_body()))
      {
        break;
      }
    }

    if (data.empty())
    {
      throw ConnectionException("empty HTTP response");
    }

    return data;
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

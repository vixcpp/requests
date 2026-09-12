/**
 *
 *  @file HttpParser.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP response parser implementation.
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

#include "http/HttpParser.hpp"
#include <vix/requests/Error.hpp>
#include "detail/CaseInsensitive.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <utility>

namespace vix::requests::http
{
  namespace
  {
    [[nodiscard]] std::string trim(std::string_view value)
    {
      std::size_t start = 0;
      while (start < value.size() &&
             std::isspace(static_cast<unsigned char>(value[start])) != 0)
      {
        ++start;
      }

      std::size_t end = value.size();
      while (end > start &&
             std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
      {
        --end;
      }

      return std::string(value.substr(start, end - start));
    }

    [[nodiscard]] bool is_interim_status(int statusCode) noexcept
    {
      return statusCode >= 100 && statusCode < 200 && statusCode != 101;
    }

    [[nodiscard]] bool status_never_has_body(int statusCode) noexcept
    {
      return (statusCode >= 100 && statusCode < 200) ||
             statusCode == 204 ||
             statusCode == 304;
    }

    [[nodiscard]] std::string strip_chunk_extension(std::string_view value)
    {
      const std::size_t semicolon = value.find(';');

      if (semicolon == std::string_view::npos)
      {
        return trim(value);
      }

      return trim(value.substr(0, semicolon));
    }

    [[nodiscard]] std::optional<std::size_t> parse_hex_size(
        std::string_view value) noexcept
    {
      if (value.empty())
      {
        return std::nullopt;
      }

      std::size_t result = 0;

      for (char raw_ch : value)
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
          return std::nullopt;
        }

        if (result >
            (std::numeric_limits<std::size_t>::max() - digit) / 16U)
        {
          return std::nullopt;
        }

        result = (result * 16U) + digit;
      }

      return result;
    }

    [[nodiscard]] std::size_t find_line_end(
        std::string_view data,
        std::size_t start) noexcept
    {
      const std::size_t crlf = data.find("\r\n", start);
      const std::size_t lf = data.find('\n', start);

      if (crlf == std::string_view::npos)
      {
        return lf;
      }

      if (lf == std::string_view::npos)
      {
        return crlf;
      }

      return crlf < lf ? crlf : lf;
    }

    [[nodiscard]] std::size_t line_separator_size(
        std::string_view data,
        std::size_t lineEnd) noexcept
    {
      if (lineEnd + 1 < data.size() &&
          data[lineEnd] == '\r' &&
          data[lineEnd + 1] == '\n')
      {
        return 2;
      }

      return 1;
    }
  } // namespace

  std::optional<std::size_t> find_header_end(
      std::string_view data) noexcept
  {
    const std::size_t crlf = data.find("\r\n\r\n");
    const std::size_t lf = data.find("\n\n");

    if (crlf == std::string_view::npos)
    {
      if (lf == std::string_view::npos)
      {
        return std::nullopt;
      }

      return lf + 2U;
    }

    if (lf == std::string_view::npos)
    {
      return crlf + 4U;
    }

    return crlf < lf ? crlf + 4U : lf + 2U;
  }

  ParsedResponseHead parse_response_head(std::string_view data)
  {
    const auto headerEnd = find_header_end(data);
    if (!headerEnd.has_value())
    {
      throw TransportException("invalid HTTP response: incomplete headers");
    }

    const std::string_view head = data.substr(0, *headerEnd);

    const std::size_t statusLineEnd = find_line_end(head, 0);
    if (statusLineEnd == std::string_view::npos)
    {
      throw TransportException("invalid HTTP response: missing status line");
    }

    const std::string statusLine = trim(head.substr(0, statusLineEnd));

    std::istringstream statusStream(statusLine);
    ParsedResponseHead parsed;
    statusStream >> parsed.version;
    statusStream >> parsed.statusCode;
    std::getline(statusStream, parsed.reason);
    parsed.reason = trim(parsed.reason);
    parsed.headerSize = *headerEnd;

    if (parsed.version.rfind("HTTP/", 0) != 0)
    {
      throw TransportException("invalid HTTP response: invalid status line");
    }

    if (parsed.statusCode < 100 || parsed.statusCode > 999)
    {
      throw TransportException("invalid HTTP response: invalid status code");
    }

    std::size_t cursor = statusLineEnd + line_separator_size(head, statusLineEnd);

    while (cursor < head.size())
    {
      const std::size_t lineEnd = find_line_end(head, cursor);
      if (lineEnd == std::string_view::npos)
      {
        break;
      }

      if (lineEnd == cursor ||
          (lineEnd == cursor + 1U && head[cursor] == '\r'))
      {
        break;
      }

      const std::string_view line = head.substr(cursor, lineEnd - cursor);
      const std::size_t colon = line.find(':');

      if (colon == std::string_view::npos)
      {
        throw TransportException("invalid HTTP response: invalid header line");
      }

      const std::string name = trim(line.substr(0, colon));
      const std::string value = detail::trim_ows(line.substr(colon + 1));

      if (name.empty())
      {
        throw TransportException("invalid HTTP response: empty header name");
      }

      parsed.headers.append(name, value);

      cursor = lineEnd + line_separator_size(head, lineEnd);
    }

    if (parsed.reason.empty())
    {
      parsed.reason = std::string(default_reason_phrase(parsed.statusCode));
    }

    return parsed;
  }

  Response parse_response(
      std::string_view data,
      std::string finalUrl,
      bool expectBody)
  {
    std::size_t offset = 0;
    ParsedResponseHead head;

    while (true)
    {
      if (offset >= data.size())
      {
        throw TransportException("invalid HTTP response: empty response");
      }

      head = parse_response_head(data.substr(offset));
      offset += head.headerSize;

      if (!is_interim_status(head.statusCode))
      {
        break;
      }
    }

    const std::string_view bodyData = data.substr(offset);

    Response response(
        std::move(finalUrl),
        head.statusCode,
        head.reason,
        head.headers,
        {});

    const BodyInfo bodyInfo = detect_body_info(
        head.statusCode,
        head.headers,
        expectBody);

    switch (bodyInfo.framing)
    {
    case BodyFraming::None:
      response.set_body({});
      break;

    case BodyFraming::ContentLength:
      if (bodyData.size() < bodyInfo.contentLength)
      {
        throw TransportException("invalid HTTP response: incomplete body");
      }

      response.set_body(
          std::string(bodyData.substr(0, bodyInfo.contentLength)));
      break;

    case BodyFraming::Chunked:
      response.set_body(decode_chunked_body(bodyData));
      break;

    case BodyFraming::ConnectionClose:
      response.set_body(std::string(bodyData));
      break;
    }

    return response;
  }

  BodyInfo detect_body_info(
      int statusCode,
      const Headers &headers,
      bool expectBody)
  {
    BodyInfo info;

    if (!expectBody || status_never_has_body(statusCode))
    {
      info.framing = BodyFraming::None;
      return info;
    }

    const auto transferEncoding = headers.get("Transfer-Encoding");
    if (transferEncoding.has_value() &&
        transfer_encoding_is_chunked(*transferEncoding))
    {
      info.framing = BodyFraming::Chunked;
      return info;
    }

    const auto contentLength = headers.get("Content-Length");
    if (contentLength.has_value())
    {
      const auto parsedLength = parse_content_length(*contentLength);

      if (!parsedLength.has_value())
      {
        throw TransportException("invalid HTTP response: invalid Content-Length");
      }

      info.framing = *parsedLength == 0U
                         ? BodyFraming::None
                         : BodyFraming::ContentLength;
      info.contentLength = *parsedLength;
      return info;
    }

    info.framing = BodyFraming::ConnectionClose;
    return info;
  }

  std::string decode_chunked_body(std::string_view data)
  {
    std::string decoded;
    std::size_t cursor = 0;

    while (cursor < data.size())
    {
      const std::size_t lineEnd = find_line_end(data, cursor);
      if (lineEnd == std::string_view::npos)
      {
        throw TransportException("invalid HTTP response: incomplete chunk size");
      }

      const std::string sizeText =
          strip_chunk_extension(data.substr(cursor, lineEnd - cursor));

      const auto chunkSize = parse_hex_size(sizeText);
      if (!chunkSize.has_value())
      {
        throw TransportException("invalid HTTP response: invalid chunk size");
      }

      cursor = lineEnd + line_separator_size(data, lineEnd);

      if (*chunkSize == 0U)
      {
        return decoded;
      }

      if (cursor + *chunkSize > data.size())
      {
        throw TransportException("invalid HTTP response: incomplete chunk body");
      }

      decoded.append(data.substr(cursor, *chunkSize));
      cursor += *chunkSize;

      if (cursor >= data.size())
      {
        throw TransportException("invalid HTTP response: missing chunk terminator");
      }

      if (cursor + 1 < data.size() &&
          data[cursor] == '\r' &&
          data[cursor + 1] == '\n')
      {
        cursor += 2;
      }
      else if (data[cursor] == '\n')
      {
        cursor += 1;
      }
      else
      {
        throw TransportException("invalid HTTP response: invalid chunk terminator");
      }
    }

    throw TransportException("invalid HTTP response: missing final chunk");
  }

  ResponseStreamDecoder::ResponseStreamDecoder(
      std::string finalUrl,
      bool expectBody,
      const RequestOptions &options)
      : finalUrl_(std::move(finalUrl)),
        expectBody_(expectBody),
        options_(options)
  {
  }

  void ResponseStreamDecoder::feed(std::span<const std::byte> bytes)
  {
    const auto *chars = reinterpret_cast<const char *>(bytes.data());
    pending_.append(chars, bytes.size());
    consume();
  }

  bool ResponseStreamDecoder::complete() const noexcept
  {
    return complete_;
  }

  bool ResponseStreamDecoder::is_http11() const noexcept
  {
    return http11_;
  }

  Response ResponseStreamDecoder::finish(bool reachedEof)
  {
    if (!headersParsed_)
    {
      throw ConnectionException("empty HTTP response");
    }

    if (bodyInfo_.framing == BodyFraming::ConnectionClose)
    {
      complete_ = reachedEof;
    }

    if (!complete_)
    {
      throw TransportException("invalid HTTP response: incomplete body");
    }

    return std::move(response_);
  }

  void ResponseStreamDecoder::emit(std::string_view bytes)
  {
    if (bytes.empty())
    {
      return;
    }

    if (discardRedirectBody_)
    {
      return;
    }

    options_.body_sink(std::span<const std::byte>(
        reinterpret_cast<const std::byte *>(bytes.data()), bytes.size()));
  }

  void ResponseStreamDecoder::consume()
  {
    while (!complete_)
    {
      if (!headersParsed_)
      {
        const auto headerEnd = find_header_end(pending_);
        if (!headerEnd.has_value())
        {
          return;
        }

        const ParsedResponseHead head = parse_response_head(pending_);
        pending_.erase(0, head.headerSize);

        if (is_interim_status(head.statusCode))
        {
          continue;
        }

        response_ = Response(
            finalUrl_, head.statusCode, head.reason, head.headers, {});
        http11_ = head.version == "HTTP/1.1";
        bodyInfo_ = detect_body_info(
            head.statusCode, head.headers, expectBody_);
        headersParsed_ = true;

        /* Client follows this redirect after the response body is consumed. */
        discardRedirectBody_ = options_.redirects_enabled() &&
            response_.is_redirect() && response_.location().has_value() &&
            !response_.location()->empty();

        if (bodyInfo_.framing == BodyFraming::None)
        {
          complete_ = true;
        }
        continue;
      }

      switch (bodyInfo_.framing)
      {
      case BodyFraming::None:
        complete_ = true;
        return;

      case BodyFraming::ContentLength:
      {
        if (remaining_ == 0U)
        {
          remaining_ = bodyInfo_.contentLength;
        }

        const std::size_t count = std::min(remaining_, pending_.size());
        if (count == 0U)
        {
          return;
        }

        emit(std::string_view(pending_).substr(0, count));
        pending_.erase(0, count);
        remaining_ -= count;
        complete_ = remaining_ == 0U;
        continue;
      }

      case BodyFraming::ConnectionClose:
        if (pending_.empty())
        {
          return;
        }
        emit(pending_);
        pending_.clear();
        return;

      case BodyFraming::Chunked:
        break;
      }

      if (chunkState_ == ChunkState::Size)
      {
        const std::size_t lineEnd = find_line_end(pending_, 0);
        if (lineEnd == std::string::npos)
        {
          return;
        }

        const std::string sizeText = strip_chunk_extension(
            std::string_view(pending_).substr(0, lineEnd));
        const auto chunkSize = parse_hex_size(sizeText);
        if (!chunkSize.has_value())
        {
          throw TransportException("invalid HTTP response: invalid chunk size");
        }

        pending_.erase(0, lineEnd + line_separator_size(pending_, lineEnd));
        if (*chunkSize == 0U)
        {
          chunkState_ = ChunkState::Trailers;
        }
        else
        {
          remaining_ = *chunkSize;
          chunkState_ = ChunkState::Data;
        }
        continue;
      }

      if (chunkState_ == ChunkState::Data)
      {
        const std::size_t count = std::min(remaining_, pending_.size());
        if (count == 0U)
        {
          return;
        }

        emit(std::string_view(pending_).substr(0, count));
        pending_.erase(0, count);
        remaining_ -= count;
        if (remaining_ == 0U)
        {
          chunkState_ = ChunkState::DataTerminator;
        }
        continue;
      }

      if (chunkState_ == ChunkState::DataTerminator)
      {
        if (pending_.empty())
        {
          return;
        }

        if (pending_.front() == '\n')
        {
          pending_.erase(0, 1U);
        }
        else if (pending_.size() >= 2U &&
                 pending_[0] == '\r' && pending_[1] == '\n')
        {
          pending_.erase(0, 2U);
        }
        else if (pending_.front() != '\r')
        {
          throw TransportException("invalid HTTP response: invalid chunk terminator");
        }
        else
        {
          return;
        }

        chunkState_ = ChunkState::Size;
        continue;
      }

      if (pending_.substr(0, 2U) == "\r\n")
      {
        pending_.erase(0, 2U);
        complete_ = true;
        return;
      }
      if (!pending_.empty() && pending_.front() == '\n')
      {
        pending_.erase(0, 1U);
        complete_ = true;
        return;
      }

      const auto trailerEnd = find_header_end(pending_);
      if (!trailerEnd.has_value())
      {
        return;
      }

      pending_.erase(0, *trailerEnd);
      complete_ = true;
    }
  }

  std::optional<std::size_t> parse_content_length(
      std::string_view value) noexcept
  {
    const std::string trimmed = trim(value);

    if (trimmed.empty())
    {
      return std::nullopt;
    }

    std::size_t result = 0;

    for (char raw_ch : trimmed)
    {
      const auto ch = static_cast<unsigned char>(raw_ch);
      if (std::isdigit(ch) == 0)
      {
        return std::nullopt;
      }

      const std::size_t digit = static_cast<std::size_t>(ch - '0');

      if (result >
          (std::numeric_limits<std::size_t>::max() - digit) / 10U)
      {
        return std::nullopt;
      }

      result = (result * 10U) + digit;
    }

    return result;
  }

  bool transfer_encoding_is_chunked(
      std::string_view value) noexcept
  {
    std::size_t start = 0;

    while (start < value.size())
    {
      const std::size_t comma = value.find(',', start);

      const std::string token = trim(
          comma == std::string_view::npos
              ? value.substr(start)
              : value.substr(start, comma - start));

      if (detail::ascii_iequals(token, "chunked"))
      {
        return true;
      }

      if (comma == std::string_view::npos)
      {
        break;
      }

      start = comma + 1U;
    }

    return false;
  }

} // namespace vix::requests::http

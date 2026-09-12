/**
 *
 *  @file HttpParser.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP response parser for the Vix requests module.
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

#ifndef VIX_REQUESTS_HTTP_HTTP_PARSER_HPP
#define VIX_REQUESTS_HTTP_HTTP_PARSER_HPP

#include <vix/requests/Headers.hpp>
#include <vix/requests/RequestOptions.hpp>
#include <vix/requests/Response.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace vix::requests::http
{
  /**
   * @brief Parsed HTTP response head.
   */
  struct ParsedResponseHead
  {
    /**
     * @brief HTTP version string.
     */
    std::string version;

    /**
     * @brief HTTP status code.
     */
    int statusCode = 0;

    /**
     * @brief HTTP reason phrase.
     */
    std::string reason;

    /**
     * @brief Response headers.
     */
    Headers headers;

    /**
     * @brief Number of bytes consumed by the response head.
     */
    std::size_t headerSize = 0;
  };

  /**
   * @brief HTTP response body framing type.
   */
  enum class BodyFraming
  {
    None,
    ContentLength,
    Chunked,
    ConnectionClose
  };

  /**
   * @brief Parsed body framing information.
   */
  struct BodyInfo
  {
    /**
     * @brief Body framing type.
     */
    BodyFraming framing = BodyFraming::None;

    /**
     * @brief Content-Length value when framing is ContentLength.
     */
    std::size_t contentLength = 0;
  };

  /**
   * @brief Incrementally parses one HTTP response for a RequestOptions sink.
   *
   * This is an internal transport helper. It accepts raw transport bytes and
   * invokes the configured body sink only with decoded body bytes.
   */
  class ResponseStreamDecoder
  {
  public:
    ResponseStreamDecoder(
        std::string finalUrl,
        bool expectBody,
        const RequestOptions &options);

    void feed(std::span<const std::byte> bytes);
    [[nodiscard]] bool complete() const noexcept;
    [[nodiscard]] bool is_http11() const noexcept;
    [[nodiscard]] Response finish(bool reachedEof);

  private:
    enum class ChunkState
    {
      Size,
      Data,
      DataTerminator,
      Trailers
    };

    void consume();
    void emit(std::string_view bytes);

    std::string finalUrl_;
    bool expectBody_{true};
    const RequestOptions &options_;
    std::string pending_;
    Response response_;
    BodyInfo bodyInfo_;
    ChunkState chunkState_{ChunkState::Size};
    std::size_t remaining_{0};
    bool headersParsed_{false};
    bool http11_{false};
    bool complete_{false};
    bool discardRedirectBody_{false};
  };

  /**
   * @brief Finds the end of an HTTP header block.
   *
   * @param data Raw HTTP data.
   * @return Offset after the header block when found.
   */
  [[nodiscard]] std::optional<std::size_t> find_header_end(
      std::string_view data) noexcept;

  /**
   * @brief Parses an HTTP response head.
   *
   * @param data Raw HTTP data beginning with a status line.
   * @return Parsed response head.
   */
  [[nodiscard]] ParsedResponseHead parse_response_head(
      std::string_view data);

  /**
   * @brief Parses a full HTTP response.
   *
   * @param data Raw HTTP response data.
   * @param finalUrl Final response URL.
   * @param expectBody Whether the request method expects a response body.
   * @return Parsed response.
   */
  [[nodiscard]] Response parse_response(
      std::string_view data,
      std::string finalUrl = {},
      bool expectBody = true);

  /**
   * @brief Detects response body framing from status code and headers.
   *
   * @param statusCode HTTP status code.
   * @param headers Response headers.
   * @param expectBody Whether the request method expects a body.
   * @return Body framing information.
   */
  [[nodiscard]] BodyInfo detect_body_info(
      int statusCode,
      const Headers &headers,
      bool expectBody);

  /**
   * @brief Decodes a chunked transfer-encoded body.
   *
   * @param data Chunked body data.
   * @return Decoded body.
   */
  [[nodiscard]] std::string decode_chunked_body(std::string_view data);

  /**
   * @brief Parses a Content-Length header value.
   *
   * @param value Header value.
   * @return Parsed content length when valid.
   */
  [[nodiscard]] std::optional<std::size_t> parse_content_length(
      std::string_view value) noexcept;

  /**
   * @brief Checks whether a Transfer-Encoding header contains chunked.
   *
   * @param value Header value.
   * @return True when chunked is present.
   */
  [[nodiscard]] bool transfer_encoding_is_chunked(
      std::string_view value) noexcept;

} // namespace vix::requests::http

#endif // VIX_REQUESTS_HTTP_HTTP_PARSER_HPP

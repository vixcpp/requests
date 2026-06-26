/**
 *
 *  @file Error.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Request exception types for the Vix requests module.
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

#ifndef VIX_REQUESTS_ERROR_HPP
#define VIX_REQUESTS_ERROR_HPP

#include <stdexcept>
#include <string>

namespace vix::requests
{
  /**
   * @brief Base exception for all request failures.
   */
  class RequestException : public std::runtime_error
  {
  public:
    /**
     * @brief Creates a request exception.
     *
     * @param message Error message.
     */
    explicit RequestException(std::string message);
  };

  /**
   * @brief Raised when a URL is invalid or unsupported.
   */
  class InvalidUrlException : public RequestException
  {
  public:
    /**
     * @brief Creates an invalid URL exception.
     *
     * @param message Error message.
     */
    explicit InvalidUrlException(std::string message);
  };

  /**
   * @brief Raised when the selected protocol is not supported.
   */
  class UnsupportedProtocolException : public RequestException
  {
  public:
    /**
     * @brief Creates an unsupported protocol exception.
     *
     * @param message Error message.
     */
    explicit UnsupportedProtocolException(std::string message);
  };

  /**
   * @brief Raised when a low-level transport failure happens.
   */
  class TransportException : public RequestException
  {
  public:
    /**
     * @brief Creates a transport exception.
     *
     * @param message Error message.
     */
    explicit TransportException(std::string message);
  };

  /**
   * @brief Raised when a connection cannot be opened or is lost.
   */
  class ConnectionException : public TransportException
  {
  public:
    /**
     * @brief Creates a connection exception.
     *
     * @param message Error message.
     */
    explicit ConnectionException(std::string message);
  };

  /**
   * @brief Raised when a request timeout is reached.
   */
  class TimeoutException : public TransportException
  {
  public:
    /**
     * @brief Creates a timeout exception.
     *
     * @param message Error message.
     */
    explicit TimeoutException(std::string message);
  };

  /**
   * @brief Raised when too many redirects are followed.
   */
  class TooManyRedirectsException : public RequestException
  {
  public:
    /**
     * @brief Creates a redirect exception.
     *
     * @param message Error message.
     */
    explicit TooManyRedirectsException(std::string message);
  };

  /**
   * @brief Raised when Response::raise_for_status() detects an HTTP error.
   */
  class HttpException : public RequestException
  {
  public:
    /**
     * @brief Creates an HTTP exception.
     *
     * @param statusCode HTTP status code.
     * @param reason HTTP reason phrase.
     * @param url Final response URL.
     */
    HttpException(
        int statusCode,
        std::string reason = {},
        std::string url = {});

    /**
     * @brief Returns the HTTP status code.
     *
     * @return Status code.
     */
    [[nodiscard]] int status_code() const noexcept;

    /**
     * @brief Returns the HTTP reason phrase.
     *
     * @return Reason phrase.
     */
    [[nodiscard]] const std::string &reason() const noexcept;

    /**
     * @brief Returns the response URL.
     *
     * @return URL.
     */
    [[nodiscard]] const std::string &url() const noexcept;

  private:
    /**
     * @brief HTTP status code.
     */
    int statusCode_{0};

    /**
     * @brief HTTP reason phrase.
     */
    std::string reason_;

    /**
     * @brief Final response URL.
     */
    std::string url_;
  };

  /**
   * @brief Builds a stable HTTP error message.
   *
   * @param statusCode HTTP status code.
   * @param reason HTTP reason phrase.
   * @param url Final response URL.
   * @return Error message.
   */
  [[nodiscard]] std::string make_http_error_message(
      int statusCode,
      const std::string &reason,
      const std::string &url);

} // namespace vix::requests

#endif // VIX_REQUESTS_ERROR_HPP

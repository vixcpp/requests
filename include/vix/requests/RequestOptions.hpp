/**
 *
 *  @file RequestOptions.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Request options for the Vix requests module.
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

#ifndef VIX_REQUESTS_REQUEST_OPTIONS_HPP
#define VIX_REQUESTS_REQUEST_OPTIONS_HPP

#include <vix/requests/Headers.hpp>
#include <vix/requests/Params.hpp>
#include <vix/requests/Timeout.hpp>
#include <vix/requests/Version.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace vix::requests
{
  /**
   * @brief Basic authentication credentials.
   */
  struct BasicAuth
  {
    /**
     * @brief Username.
     */
    std::string username;

    /**
     * @brief Password.
     */
    std::string password;

    /**
     * @brief Checks whether credentials are configured.
     *
     * @return True when username or password is not empty.
     */
    [[nodiscard]] bool configured() const noexcept;
  };

  /**
   * @brief Request behavior options.
   *
   * This object is intentionally simple and public so callers can use it in a
   * Python-requests-like way:
   *
   * @code
   * RequestOptions options;
   * options.headers.set("Accept", "application/json");
   * options.params.set("page", "1");
   * options.timeout = std::chrono::seconds(10);
   * @endcode
   */
  struct RequestOptions
  {
    /**
     * @brief Extra request headers.
     */
    Headers headers;

    /**
     * @brief Query parameters appended to the URL.
     */
    Params params;

    /**
     * @brief Timeout configuration.
     */
    Timeout timeout;

    /**
     * @brief Basic authentication credentials.
     */
    BasicAuth auth;

    /**
     * @brief Follow HTTP redirects automatically.
     */
    bool follow_redirects = true;

    /**
     * @brief Maximum number of redirects.
     */
    std::size_t max_redirects = 10;

    /**
     * @brief Verify TLS certificates when TLS support exists.
     */
    bool verify_tls = true;

    /**
     * @brief Keep the connection reusable when possible.
     */
    bool keep_alive = true;

    /**
     * @brief User-Agent sent when no explicit User-Agent header exists.
     */
    std::string user_agent = std::string(defaultUserAgent);

    /**
     * @brief Optional Host header override.
     */
    std::optional<std::string> host_override;

    /**
     * @brief Checks whether redirects are enabled.
     *
     * @return True when redirects are enabled and max_redirects is not zero.
     */
    [[nodiscard]] bool redirects_enabled() const noexcept;

    /**
     * @brief Checks whether a user agent is configured.
     *
     * @return True when user_agent is not empty.
     */
    [[nodiscard]] bool has_user_agent() const noexcept;

    /**
     * @brief Checks whether a Host override is configured.
     *
     * @return True when host_override has a value.
     */
    [[nodiscard]] bool has_host_override() const noexcept;

    /**
     * @brief Sets basic authentication.
     *
     * @param username Username.
     * @param password Password.
     * @return This options object.
     */
    RequestOptions &set_basic_auth(
        std::string username,
        std::string password);

    /**
     * @brief Sets a request timeout for all phases.
     *
     * @param value Timeout value.
     * @return This options object.
     */
    RequestOptions &set_timeout(Timeout::Duration value);

    /**
     * @brief Sets the User-Agent value.
     *
     * @param value User-Agent value.
     * @return This options object.
     */
    RequestOptions &set_user_agent(std::string value);

    /**
     * @brief Sets a Host header override.
     *
     * @param value Host value.
     * @return This options object.
     */
    RequestOptions &set_host_override(std::string value);

    /**
     * @brief Clears the Host header override.
     *
     * @return This options object.
     */
    RequestOptions &clear_host_override();
  };

  /**
   * @brief Merges two request option objects.
   *
   * Values from overrideOptions replace or extend baseOptions.
   *
   * @param baseOptions Base options.
   * @param overrideOptions Override options.
   * @return Merged options.
   */
  [[nodiscard]] RequestOptions merge_request_options(
      const RequestOptions &baseOptions,
      const RequestOptions &overrideOptions);

  /**
   * @brief Checks whether options contain a header.
   *
   * @param options Request options.
   * @param name Header name.
   * @return True when the header exists.
   */
  [[nodiscard]] bool has_option_header(
      const RequestOptions &options,
      std::string_view name) noexcept;

} // namespace vix::requests

#endif // VIX_REQUESTS_REQUEST_OPTIONS_HPP

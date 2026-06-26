/**
 *
 *  @file CookieJar.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Cookie storage helpers for the Vix requests module.
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

#ifndef VIX_REQUESTS_HTTP_COOKIE_JAR_HPP
#define VIX_REQUESTS_HTTP_COOKIE_JAR_HPP

#include <vix/requests/Headers.hpp>
#include <vix/requests/Url.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vix::requests::http
{
  /**
   * @brief HTTP cookie entry.
   */
  struct Cookie
  {
    using Clock = std::chrono::system_clock;

    /**
     * @brief Cookie name.
     */
    std::string name;

    /**
     * @brief Cookie value.
     */
    std::string value;

    /**
     * @brief Cookie domain.
     */
    std::string domain;

    /**
     * @brief Cookie path.
     */
    std::string path = "/";

    /**
     * @brief SameSite attribute value.
     */
    std::string sameSite;

    /**
     * @brief True when the cookie only belongs to the exact origin host.
     */
    bool hostOnly = true;

    /**
     * @brief True when the cookie requires HTTPS.
     */
    bool secure = false;

    /**
     * @brief True when the cookie is HttpOnly.
     */
    bool httpOnly = false;

    /**
     * @brief Optional expiration time.
     */
    std::optional<Clock::time_point> expiresAt;

    /**
     * @brief Checks whether the cookie is expired.
     *
     * @param now Current time.
     * @return True when expired.
     */
    [[nodiscard]] bool expired(Clock::time_point now) const noexcept;

    /**
     * @brief Checks whether the cookie has an explicit expiration.
     *
     * @return True when persistent.
     */
    [[nodiscard]] bool persistent() const noexcept;
  };

  /**
   * @brief Simple in-memory cookie jar.
   */
  class CookieJar
  {
  public:
    using Container = std::vector<Cookie>;

    /**
     * @brief Stores cookies from response headers.
     *
     * @param url Response URL.
     * @param headers Response headers.
     */
    void store_from_response(
        const Url &url,
        const Headers &headers);

    /**
     * @brief Stores a single Set-Cookie header.
     *
     * @param url Response URL.
     * @param setCookieHeader Set-Cookie value.
     */
    void store(
        const Url &url,
        std::string_view setCookieHeader);

    /**
     * @brief Builds the Cookie header value for a request URL.
     *
     * @param url Request URL.
     * @return Cookie header value.
     */
    [[nodiscard]] std::string cookie_header_for_url(const Url &url) const;

    /**
     * @brief Adds a Cookie header to headers when matching cookies exist.
     *
     * @param url Request URL.
     * @param headers Request headers.
     */
    void apply_to(
        const Url &url,
        Headers &headers) const;

    /**
     * @brief Removes expired cookies.
     */
    void remove_expired();

    /**
     * @brief Removes all cookies.
     */
    void clear() noexcept;

    /**
     * @brief Returns true when there are no stored cookies.
     *
     * @return True when empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the number of stored cookies.
     *
     * @return Cookie count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns all cookies.
     *
     * @return Stored cookies.
     */
    [[nodiscard]] const Container &cookies() const noexcept;

  private:
    /**
     * @brief Stored cookies.
     */
    Container cookies_;

    /**
     * @brief Inserts or replaces a cookie.
     *
     * @param cookie Cookie.
     */
    void upsert(Cookie cookie);
  };

  /**
   * @brief Parses one Set-Cookie header value.
   *
   * @param url Response URL.
   * @param value Set-Cookie value.
   * @return Parsed cookie.
   */
  [[nodiscard]] std::optional<Cookie> parse_set_cookie(
      const Url &url,
      std::string_view value);

  /**
   * @brief Checks whether a cookie matches a request URL.
   *
   * @param cookie Cookie.
   * @param url Request URL.
   * @return True when the cookie should be sent.
   */
  [[nodiscard]] bool cookie_matches_url(
      const Cookie &cookie,
      const Url &url);

} // namespace vix::requests::http

#endif // VIX_REQUESTS_HTTP_COOKIE_JAR_HPP

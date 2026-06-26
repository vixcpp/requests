/**
 *
 *  @file Url.hpp
 *  @author Gaspard Kirira
 *
 *  @brief URL parser for the Vix requests module.
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

#ifndef VIX_REQUESTS_URL_HPP
#define VIX_REQUESTS_URL_HPP

#include <vix/requests/Params.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace vix::requests
{
  /**
   * @brief Parsed absolute URL.
   */
  class Url
  {
  public:
    /**
     * @brief Creates an empty URL.
     */
    Url() = default;

    /**
     * @brief Parses an absolute URL.
     *
     * @param value URL string.
     * @return Parsed URL.
     */
    [[nodiscard]] static Url parse(std::string_view value);

    /**
     * @brief Returns the URL scheme.
     *
     * @return Scheme.
     */
    [[nodiscard]] const std::string &scheme() const noexcept;

    /**
     * @brief Returns the URL host.
     *
     * @return Host.
     */
    [[nodiscard]] const std::string &host() const noexcept;

    /**
     * @brief Returns the effective port.
     *
     * @return Port.
     */
    [[nodiscard]] std::uint16_t port() const noexcept;

    /**
     * @brief Returns the explicit port when one was provided.
     *
     * @return Explicit port.
     */
    [[nodiscard]] std::optional<std::uint16_t> explicit_port() const noexcept;

    /**
     * @brief Checks whether the URL had an explicit port.
     *
     * @return True when an explicit port was provided.
     */
    [[nodiscard]] bool has_explicit_port() const noexcept;

    /**
     * @brief Returns the path.
     *
     * @return Path.
     */
    [[nodiscard]] const std::string &path() const noexcept;

    /**
     * @brief Returns the query string without '?'.
     *
     * @return Query string.
     */
    [[nodiscard]] const std::string &query() const noexcept;

    /**
     * @brief Returns the fragment without '#'.
     *
     * Fragments are stored but ignored when building HTTP request targets.
     *
     * @return Fragment.
     */
    [[nodiscard]] const std::string &fragment() const noexcept;

    /**
     * @brief Checks whether the scheme is http.
     *
     * @return True when scheme is http.
     */
    [[nodiscard]] bool is_http() const noexcept;

    /**
     * @brief Checks whether the scheme is https.
     *
     * @return True when scheme is https.
     */
    [[nodiscard]] bool is_https() const noexcept;

    /**
     * @brief Checks whether the URL has a query string.
     *
     * @return True when query is not empty.
     */
    [[nodiscard]] bool has_query() const noexcept;

    /**
     * @brief Returns the authority part used by HTTP.
     *
     * @return host or host:port.
     */
    [[nodiscard]] std::string authority() const;

    /**
     * @brief Returns the origin.
     *
     * @return scheme://authority.
     */
    [[nodiscard]] std::string origin() const;

    /**
     * @brief Returns the HTTP request target.
     *
     * This includes path and query, but never the fragment.
     *
     * @return Request target.
     */
    [[nodiscard]] std::string request_target() const;

    /**
     * @brief Returns the full URL without fragment.
     *
     * @return URL string.
     */
    [[nodiscard]] std::string without_fragment() const;

    /**
     * @brief Returns the full URL string.
     *
     * @return URL string.
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief Returns a copy with extra query parameters appended.
     *
     * @param params Query parameters.
     * @return URL with appended params.
     */
    [[nodiscard]] Url with_params(const Params &params) const;

  private:
    std::string scheme_;
    std::string host_;
    std::uint16_t port_{0};
    std::optional<std::uint16_t> explicitPort_;
    std::string path_{"/"};
    std::string query_;
    std::string fragment_;

    static void validate_scheme(std::string_view scheme);
    static std::uint16_t default_port_for_scheme(std::string_view scheme);
  };

  /**
   * @brief Parses an absolute URL.
   *
   * @param value URL string.
   * @return Parsed URL.
   */
  [[nodiscard]] Url parse_url(std::string_view value);

  /**
   * @brief Appends a query string to an existing URL string.
   *
   * @param url URL string.
   * @param query Query string without '?'.
   * @return URL with appended query.
   */
  [[nodiscard]] std::string append_query_string(
      std::string_view url,
      std::string_view query);

} // namespace vix::requests

#endif // VIX_REQUESTS_URL_HPP

/**
 *
 *  @file RedirectPolicy.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Redirect handling helpers for the Vix requests module.
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

#ifndef VIX_REQUESTS_HTTP_REDIRECT_POLICY_HPP
#define VIX_REQUESTS_HTTP_REDIRECT_POLICY_HPP

#include <vix/requests/Request.hpp>
#include <vix/requests/Response.hpp>
#include <vix/requests/Url.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace vix::requests::http
{
  /**
   * @brief Redirect decision result.
   */
  struct RedirectDecision
  {
    /**
     * @brief True when a redirect should be followed.
     */
    bool follow = false;

    /**
     * @brief Target redirect URL.
     */
    std::string url;

    /**
     * @brief Method to use for the next request.
     */
    std::string method;

    /**
     * @brief Whether the next request should keep the original body.
     */
    bool keepBody = false;
  };

  /**
   * @brief Redirect history tracker.
   */
  class RedirectHistory
  {
  public:
    /**
     * @brief Adds a visited URL to the redirect history.
     *
     * @param url URL.
     */
    void add(std::string url);

    /**
     * @brief Checks whether a URL already exists in the history.
     *
     * @param url URL.
     * @return True when already visited.
     */
    [[nodiscard]] bool contains(std::string_view url) const noexcept;

    /**
     * @brief Returns the number of visited redirects.
     *
     * @return Redirect count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Clears redirect history.
     */
    void clear() noexcept;

    /**
     * @brief Returns all visited redirect URLs.
     *
     * @return URLs.
     */
    [[nodiscard]] const std::vector<std::string> &entries() const noexcept;

  private:
    /**
     * @brief Visited redirect URLs.
     */
    std::vector<std::string> entries_;
  };

  /**
   * @brief Computes whether a response should be followed as a redirect.
   *
   * @param request Current request.
   * @param response Current response.
   * @param history Redirect history.
   * @return Redirect decision.
   */
  [[nodiscard]] RedirectDecision decide_redirect(
      const Request &request,
      const Response &response,
      const RedirectHistory &history);

  /**
   * @brief Builds the next request for a redirect.
   *
   * @param current Current request.
   * @param decision Redirect decision.
   * @return Next request.
   */
  [[nodiscard]] Request make_redirect_request(
      const Request &current,
      const RedirectDecision &decision);

  /**
   * @brief Resolves a redirect Location value against a base URL.
   *
   * Supports absolute URLs, protocol-relative URLs, root-relative paths, and
   * relative paths.
   *
   * @param base Base URL.
   * @param location Location header value.
   * @return Resolved absolute URL.
   */
  [[nodiscard]] std::string resolve_redirect_url(
      const Url &base,
      std::string_view location);

  /**
   * @brief Checks whether a status code redirects with GET.
   *
   * @param statusCode HTTP status code.
   * @param currentMethod Current method.
   * @return True when the next method should be GET.
   */
  [[nodiscard]] bool redirect_rewrites_method_to_get(
      int statusCode,
      std::string_view currentMethod) noexcept;

  /**
   * @brief Checks whether a redirect status preserves method and body.
   *
   * @param statusCode HTTP status code.
   * @return True for 307 and 308.
   */
  [[nodiscard]] bool redirect_preserves_method_and_body(
      int statusCode) noexcept;

} // namespace vix::requests::http

#endif // VIX_REQUESTS_HTTP_REDIRECT_POLICY_HPP

/**
 *
 *  @file Headers.hpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP header container for the Vix requests module.
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

#ifndef VIX_REQUESTS_HEADERS_HPP
#define VIX_REQUESTS_HEADERS_HPP

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::requests
{
  /**
   * @brief Single HTTP header entry.
   */
  struct Header
  {
    /**
     * @brief Header name with its original casing.
     */
    std::string name;

    /**
     * @brief Header value.
     */
    std::string value;
  };

  /**
   * @brief Case-insensitive HTTP headers container.
   *
   * Header lookups are case-insensitive, but inserted names keep their
   * original casing. This keeps the API convenient while preserving readable
   * serialized output.
   */
  class Headers
  {
  public:
    using Container = std::vector<Header>;
    using iterator = Container::iterator;
    using const_iterator = Container::const_iterator;

    /**
     * @brief Creates an empty headers container.
     */
    Headers() = default;

    /**
     * @brief Creates headers from key-value pairs.
     *
     * @param values Header key-value pairs.
     */
    Headers(std::initializer_list<std::pair<std::string, std::string>> values);

    /**
     * @brief Sets a header value.
     *
     * If the header already exists, only the value is replaced and the first
     * original header name casing is preserved.
     *
     * @param name Header name.
     * @param value Header value.
     */
    void set(std::string_view name, std::string_view value);

    /**
     * @brief Appends a header value without replacing existing entries.
     *
     * This is useful for response headers such as Set-Cookie.
     *
     * @param name Header name.
     * @param value Header value.
     */
    void append(std::string_view name, std::string_view value);

    /**
     * @brief Returns the first value for a header.
     *
     * @param name Header name.
     * @return Header value when present.
     */
    [[nodiscard]] std::optional<std::string> get(
        std::string_view name) const;

    /**
     * @brief Returns all values for a header.
     *
     * @param name Header name.
     * @return All matching header values.
     */
    [[nodiscard]] std::vector<std::string> get_all(
        std::string_view name) const;

    /**
     * @brief Checks whether a header exists.
     *
     * @param name Header name.
     * @return True when the header exists.
     */
    [[nodiscard]] bool has(std::string_view name) const noexcept;

    /**
     * @brief Removes all headers matching the given name.
     *
     * @param name Header name.
     * @return Number of removed entries.
     */
    std::size_t remove(std::string_view name);

    /**
     * @brief Clears all headers.
     */
    void clear() noexcept;

    /**
     * @brief Returns true when there are no headers.
     *
     * @return True when empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the number of stored header entries.
     *
     * @return Header count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns all header entries.
     *
     * @return Header entries.
     */
    [[nodiscard]] const Container &entries() const noexcept;

    /**
     * @brief Returns a mutable iterator to the first header.
     */
    [[nodiscard]] iterator begin() noexcept;

    /**
     * @brief Returns a mutable iterator past the last header.
     */
    [[nodiscard]] iterator end() noexcept;

    /**
     * @brief Returns a read-only iterator to the first header.
     */
    [[nodiscard]] const_iterator begin() const noexcept;

    /**
     * @brief Returns a read-only iterator past the last header.
     */
    [[nodiscard]] const_iterator end() const noexcept;

    /**
     * @brief Returns a read-only iterator to the first header.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept;

    /**
     * @brief Returns a read-only iterator past the last header.
     */
    [[nodiscard]] const_iterator cend() const noexcept;

  private:
    /**
     * @brief Stored header entries.
     */
    Container entries_;

    /**
     * @brief Finds the first matching header.
     *
     * @param name Header name.
     * @return Index when found.
     */
    [[nodiscard]] std::optional<std::size_t> find_index(
        std::string_view name) const noexcept;

    /**
     * @brief Validates a header name.
     *
     * @param name Header name.
     */
    static void validate_name(std::string_view name);
  };

} // namespace vix::requests

#endif // VIX_REQUESTS_HEADERS_HPP

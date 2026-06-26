/**
 *
 *  @file Params.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Query parameter container for the Vix requests module.
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

#ifndef VIX_REQUESTS_PARAMS_HPP
#define VIX_REQUESTS_PARAMS_HPP

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::requests
{
  /**
   * @brief Single query parameter entry.
   */
  struct Param
  {
    /**
     * @brief Parameter name.
     */
    std::string name;

    /**
     * @brief Parameter value.
     */
    std::string value;
  };

  /**
   * @brief Ordered query parameter container.
   *
   * Params preserves insertion order and supports duplicate keys through
   * append(). set() replaces the first matching key and removes duplicates.
   */
  class Params
  {
  public:
    using Container = std::vector<Param>;
    using iterator = Container::iterator;
    using const_iterator = Container::const_iterator;

    /**
     * @brief Creates an empty params container.
     */
    Params() = default;

    /**
     * @brief Creates params from key-value pairs.
     *
     * @param values Parameter key-value pairs.
     */
    Params(std::initializer_list<std::pair<std::string, std::string>> values);

    /**
     * @brief Sets a parameter value.
     *
     * If the key already exists, the first value is replaced and duplicated
     * entries with the same name are removed.
     *
     * @param name Parameter name.
     * @param value Parameter value.
     */
    void set(std::string_view name, std::string_view value);

    /**
     * @brief Appends a parameter without removing existing entries.
     *
     * @param name Parameter name.
     * @param value Parameter value.
     */
    void append(std::string_view name, std::string_view value);

    /**
     * @brief Returns the first value for a parameter.
     *
     * @param name Parameter name.
     * @return Parameter value when present.
     */
    [[nodiscard]] std::optional<std::string> get(
        std::string_view name) const;

    /**
     * @brief Returns all values for a parameter.
     *
     * @param name Parameter name.
     * @return All matching values.
     */
    [[nodiscard]] std::vector<std::string> get_all(
        std::string_view name) const;

    /**
     * @brief Checks whether a parameter exists.
     *
     * @param name Parameter name.
     * @return True when the parameter exists.
     */
    [[nodiscard]] bool has(std::string_view name) const noexcept;

    /**
     * @brief Removes all parameters matching the given name.
     *
     * @param name Parameter name.
     * @return Number of removed entries.
     */
    std::size_t remove(std::string_view name);

    /**
     * @brief Clears all parameters.
     */
    void clear() noexcept;

    /**
     * @brief Returns true when there are no parameters.
     *
     * @return True when empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Returns the number of stored parameter entries.
     *
     * @return Parameter count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Builds an encoded query string without the leading '?'.
     *
     * @return Encoded query string.
     */
    [[nodiscard]] std::string to_query_string() const;

    /**
     * @brief Returns all parameter entries.
     *
     * @return Parameter entries.
     */
    [[nodiscard]] const Container &entries() const noexcept;

    /**
     * @brief Returns a mutable iterator to the first parameter.
     */
    [[nodiscard]] iterator begin() noexcept;

    /**
     * @brief Returns a mutable iterator past the last parameter.
     */
    [[nodiscard]] iterator end() noexcept;

    /**
     * @brief Returns a read-only iterator to the first parameter.
     */
    [[nodiscard]] const_iterator begin() const noexcept;

    /**
     * @brief Returns a read-only iterator past the last parameter.
     */
    [[nodiscard]] const_iterator end() const noexcept;

    /**
     * @brief Returns a read-only iterator to the first parameter.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept;

    /**
     * @brief Returns a read-only iterator past the last parameter.
     */
    [[nodiscard]] const_iterator cend() const noexcept;

  private:
    /**
     * @brief Stored parameter entries.
     */
    Container entries_;

    /**
     * @brief Finds the first matching parameter.
     *
     * @param name Parameter name.
     * @return Index when found.
     */
    [[nodiscard]] std::optional<std::size_t> find_index(
        std::string_view name) const noexcept;
  };

  /**
   * @brief Encodes a value for URL query usage.
   *
   * @param value Input value.
   * @return Encoded value.
   */
  [[nodiscard]] std::string url_encode(std::string_view value);

  /**
   * @brief Decodes a value from URL query usage.
   *
   * @param value Encoded value.
   * @return Decoded value.
   */
  [[nodiscard]] std::string url_decode(std::string_view value);

  /**
   * @brief Encodes a value for application/x-www-form-urlencoded usage.
   *
   * @param value Input value.
   * @return Encoded value.
   */
  [[nodiscard]] std::string form_url_encode(std::string_view value);

  /**
   * @brief Decodes a value from application/x-www-form-urlencoded usage.
   *
   * @param value Encoded value.
   * @return Decoded value.
   */
  [[nodiscard]] std::string form_url_decode(std::string_view value);

  /**
   * @brief Builds an encoded query string from params.
   *
   * @param params Query parameters.
   * @return Encoded query string without leading '?'.
   */
  [[nodiscard]] std::string build_query_string(const Params &params);

} // namespace vix::requests

#endif // VIX_REQUESTS_PARAMS_HPP

/**
 *
 *  @file Timeout.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Timeout configuration for the Vix requests module.
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

#ifndef VIX_REQUESTS_TIMEOUT_HPP
#define VIX_REQUESTS_TIMEOUT_HPP

#include <chrono>

namespace vix::requests
{
  /**
   * @brief Timeout configuration for an HTTP request.
   *
   * A duration of zero means that no explicit timeout is configured for that
   * phase. The transport layer can still apply platform defaults.
   */
  class Timeout
  {
  public:
    using Duration = std::chrono::milliseconds;

    /**
     * @brief Creates a timeout with no explicit limits.
     */
    Timeout() = default;

    /**
     * @brief Creates a timeout using the same value for all phases.
     *
     * @param value Timeout duration.
     */
    explicit Timeout(Duration value);

    /**
     * @brief Creates a timeout with separate phase values.
     *
     * @param connect Connect timeout.
     * @param read Read timeout.
     * @param total Total timeout.
     */
    Timeout(
        Duration connect,
        Duration read,
        Duration total);

    /**
     * @brief Assigns the same timeout value to all phases.
     *
     * @param value Timeout duration.
     * @return This timeout.
     */
    Timeout &operator=(Duration value);

    /**
     * @brief Creates a timeout from seconds.
     *
     * @param seconds Timeout duration in seconds.
     * @return Timeout.
     */
    [[nodiscard]] static Timeout seconds(long seconds);

    /**
     * @brief Creates a timeout from milliseconds.
     *
     * @param milliseconds Timeout duration in milliseconds.
     * @return Timeout.
     */
    [[nodiscard]] static Timeout milliseconds(long milliseconds);

    /**
     * @brief Creates a timeout with no explicit limits.
     *
     * @return Timeout.
     */
    [[nodiscard]] static Timeout none();

    /**
     * @brief Returns the connect timeout.
     *
     * @return Connect timeout.
     */
    [[nodiscard]] Duration connect() const noexcept;

    /**
     * @brief Returns the read timeout.
     *
     * @return Read timeout.
     */
    [[nodiscard]] Duration read() const noexcept;

    /**
     * @brief Returns the total timeout.
     *
     * @return Total timeout.
     */
    [[nodiscard]] Duration total() const noexcept;

    /**
     * @brief Sets the connect timeout.
     *
     * @param value Timeout duration.
     */
    void set_connect(Duration value);

    /**
     * @brief Sets the read timeout.
     *
     * @param value Timeout duration.
     */
    void set_read(Duration value);

    /**
     * @brief Sets the total timeout.
     *
     * @param value Timeout duration.
     */
    void set_total(Duration value);

    /**
     * @brief Checks whether a connect timeout is configured.
     *
     * @return True when connect timeout is greater than zero.
     */
    [[nodiscard]] bool has_connect() const noexcept;

    /**
     * @brief Checks whether a read timeout is configured.
     *
     * @return True when read timeout is greater than zero.
     */
    [[nodiscard]] bool has_read() const noexcept;

    /**
     * @brief Checks whether a total timeout is configured.
     *
     * @return True when total timeout is greater than zero.
     */
    [[nodiscard]] bool has_total() const noexcept;

    /**
     * @brief Checks whether any timeout is configured.
     *
     * @return True when at least one timeout is greater than zero.
     */
    [[nodiscard]] bool active() const noexcept;

  private:
    /**
     * @brief Connect timeout.
     */
    Duration connect_{Duration{0}};

    /**
     * @brief Read timeout.
     */
    Duration read_{Duration{0}};

    /**
     * @brief Total timeout.
     */
    Duration total_{Duration{0}};

    /**
     * @brief Validates a timeout duration.
     *
     * @param value Timeout duration.
     */
    static void validate(Duration value);
  };

} // namespace vix::requests

#endif // VIX_REQUESTS_TIMEOUT_HPP

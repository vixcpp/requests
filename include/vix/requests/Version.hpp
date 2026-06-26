/**
 *
 *  @file Version.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Version information for the Vix requests module.
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

#ifndef VIX_REQUESTS_VERSION_HPP
#define VIX_REQUESTS_VERSION_HPP

#include <string>
#include <string_view>

namespace vix::requests
{
  /**
   * @brief Major version.
   */
  inline constexpr int versionMajor = 1;

  /**
   * @brief Minor version.
   */
  inline constexpr int versionMinor = 0;

  /**
   * @brief Patch version.
   */
  inline constexpr int versionPatch = 0;

  /**
   * @brief Stable version string.
   */
  inline constexpr std::string_view versionString = "1.0.0";

  /**
   * @brief Default User-Agent used by the requests module.
   */
  inline constexpr std::string_view defaultUserAgent = "vix-requests/1.0.0";

  /**
   * @brief Returns the major version.
   *
   * @return Major version.
   */
  [[nodiscard]] int version_major() noexcept;

  /**
   * @brief Returns the minor version.
   *
   * @return Minor version.
   */
  [[nodiscard]] int version_minor() noexcept;

  /**
   * @brief Returns the patch version.
   *
   * @return Patch version.
   */
  [[nodiscard]] int version_patch() noexcept;

  /**
   * @brief Returns the stable version string.
   *
   * @return Version string.
   */
  [[nodiscard]] std::string_view version() noexcept;

  /**
   * @brief Returns the default User-Agent.
   *
   * @return User-Agent string.
   */
  [[nodiscard]] std::string_view user_agent() noexcept;

  /**
   * @brief Returns a full module name with version.
   *
   * @return Full version name.
   */
  [[nodiscard]] std::string version_name();

} // namespace vix::requests

#endif // VIX_REQUESTS_VERSION_HPP

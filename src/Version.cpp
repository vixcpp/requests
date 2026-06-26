/**
 *
 *  @file Version.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Version information implementation for the Vix requests module.
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

#include <vix/requests/Version.hpp>

namespace vix::requests
{
  int version_major() noexcept
  {
    return versionMajor;
  }

  int version_minor() noexcept
  {
    return versionMinor;
  }

  int version_patch() noexcept
  {
    return versionPatch;
  }

  std::string_view version() noexcept
  {
    return versionString;
  }

  std::string_view user_agent() noexcept
  {
    return defaultUserAgent;
  }

  std::string version_name()
  {
    return "vix-requests/" + std::string(versionString);
  }

} // namespace vix::requests

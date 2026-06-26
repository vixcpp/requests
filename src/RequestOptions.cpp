/**
 *
 *  @file RequestOptions.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Request options implementation for the Vix requests module.
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

#include <vix/requests/RequestOptions.hpp>

#include <utility>

namespace vix::requests
{
  bool BasicAuth::configured() const noexcept
  {
    return !username.empty() || !password.empty();
  }

  bool RequestOptions::redirects_enabled() const noexcept
  {
    return follow_redirects && max_redirects > 0;
  }

  bool RequestOptions::has_user_agent() const noexcept
  {
    return !user_agent.empty();
  }

  bool RequestOptions::has_host_override() const noexcept
  {
    return host_override.has_value() && !host_override->empty();
  }

  RequestOptions &RequestOptions::set_basic_auth(
      std::string username,
      std::string password)
  {
    auth.username = std::move(username);
    auth.password = std::move(password);
    return *this;
  }

  RequestOptions &RequestOptions::set_timeout(Timeout::Duration value)
  {
    timeout = value;
    return *this;
  }

  RequestOptions &RequestOptions::set_user_agent(std::string value)
  {
    user_agent = std::move(value);
    return *this;
  }

  RequestOptions &RequestOptions::set_host_override(std::string value)
  {
    host_override = std::move(value);
    return *this;
  }

  RequestOptions &RequestOptions::clear_host_override()
  {
    host_override.reset();
    return *this;
  }

  RequestOptions merge_request_options(
      const RequestOptions &baseOptions,
      const RequestOptions &overrideOptions)
  {
    RequestOptions merged = baseOptions;

    for (const auto &entry : overrideOptions.headers.entries())
    {
      merged.headers.set(entry.name, entry.value);
    }

    for (const auto &entry : overrideOptions.params.entries())
    {
      merged.params.set(entry.name, entry.value);
    }

    if (overrideOptions.timeout.active())
    {
      merged.timeout = overrideOptions.timeout;
    }

    if (overrideOptions.auth.configured())
    {
      merged.auth = overrideOptions.auth;
    }

    merged.follow_redirects = overrideOptions.follow_redirects;
    merged.max_redirects = overrideOptions.max_redirects;
    merged.verify_tls = overrideOptions.verify_tls;
    merged.keep_alive = overrideOptions.keep_alive;

    if (!overrideOptions.user_agent.empty())
    {
      merged.user_agent = overrideOptions.user_agent;
    }

    if (overrideOptions.host_override.has_value())
    {
      merged.host_override = overrideOptions.host_override;
    }

    return merged;
  }

  bool has_option_header(
      const RequestOptions &options,
      std::string_view name) noexcept
  {
    return options.headers.has(name);
  }

} // namespace vix::requests

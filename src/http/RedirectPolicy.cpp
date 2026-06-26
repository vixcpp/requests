/**
 *
 *  @file RedirectPolicy.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Redirect handling helper implementation.
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

#include "http/RedirectPolicy.hpp"
#include <vix/requests/Error.hpp>
#include "detail/CaseInsensitive.hpp"

#include <algorithm>
#include <utility>

namespace vix::requests::http
{
  namespace
  {
    [[nodiscard]] std::string trim_location(std::string_view value)
    {
      return detail::trim_ows(value);
    }

    [[nodiscard]] bool starts_with(
        std::string_view value,
        std::string_view prefix) noexcept
    {
      return value.size() >= prefix.size() &&
             value.substr(0, prefix.size()) == prefix;
    }

    [[nodiscard]] std::string directory_path(std::string_view path)
    {
      if (path.empty() || path.front() != '/')
      {
        return "/";
      }

      const std::size_t slash = path.rfind('/');
      if (slash == std::string_view::npos || slash == 0)
      {
        return "/";
      }

      return std::string(path.substr(0, slash + 1U));
    }

    [[nodiscard]] std::string normalize_path(std::string_view path)
    {
      std::vector<std::string> parts;
      std::size_t cursor = 0;

      while (cursor <= path.size())
      {
        const std::size_t slash = path.find('/', cursor);
        const std::string_view segment =
            slash == std::string_view::npos
                ? path.substr(cursor)
                : path.substr(cursor, slash - cursor);

        if (segment.empty() || segment == ".")
        {
        }
        else if (segment == "..")
        {
          if (!parts.empty())
          {
            parts.pop_back();
          }
        }
        else
        {
          parts.emplace_back(segment);
        }

        if (slash == std::string_view::npos)
        {
          break;
        }

        cursor = slash + 1U;
      }

      std::string normalized = "/";

      for (std::size_t index = 0; index < parts.size(); ++index)
      {
        if (index > 0)
        {
          normalized.push_back('/');
        }

        normalized += parts[index];
      }

      if (!path.empty() && path.back() == '/' && normalized.back() != '/')
      {
        normalized.push_back('/');
      }

      return normalized;
    }
  } // namespace

  void RedirectHistory::add(std::string url)
  {
    entries_.push_back(std::move(url));
  }

  bool RedirectHistory::contains(std::string_view url) const noexcept
  {
    return std::any_of(
        entries_.begin(),
        entries_.end(),
        [url](const std::string &entry)
        {
          return entry == url;
        });
  }

  std::size_t RedirectHistory::size() const noexcept
  {
    return entries_.size();
  }

  void RedirectHistory::clear() noexcept
  {
    entries_.clear();
  }

  const std::vector<std::string> &RedirectHistory::entries() const noexcept
  {
    return entries_;
  }

  RedirectDecision decide_redirect(
      const Request &request,
      const Response &response,
      const RedirectHistory &history)
  {
    RedirectDecision decision;

    if (!request.options().redirects_enabled())
    {
      return decision;
    }

    if (!response.is_redirect())
    {
      return decision;
    }

    const auto location = response.location();
    if (!location.has_value() || location->empty())
    {
      return decision;
    }

    if (history.size() >= request.options().max_redirects)
    {
      throw TooManyRedirectsException("too many redirects");
    }

    decision.url = resolve_redirect_url(
        request.final_url(),
        trim_location(*location));

    if (history.contains(decision.url))
    {
      throw TooManyRedirectsException("redirect loop detected");
    }

    decision.follow = true;

    if (redirect_rewrites_method_to_get(
            response.status_code(),
            request.method()))
    {
      decision.method = "GET";
      decision.keepBody = false;
    }
    else
    {
      decision.method = request.method();
      decision.keepBody = redirect_preserves_method_and_body(
          response.status_code());
    }

    return decision;
  }

  Request make_redirect_request(
      const Request &current,
      const RedirectDecision &decision)
  {
    RequestOptions options = current.options();

    /*
     * Params were already applied to the previous URL. For redirect targets,
     * the Location header is authoritative.
     */
    options.params.clear();

    Body body;
    if (decision.keepBody)
    {
      body = current.body();
    }

    return Request(
        decision.method,
        decision.url,
        std::move(options),
        std::move(body));
  }

  std::string resolve_redirect_url(
      const Url &base,
      std::string_view location)
  {
    const std::string target = trim_location(location);

    if (target.empty())
    {
      throw InvalidUrlException("invalid redirect URL: empty Location");
    }

    if (starts_with(target, "http://") ||
        starts_with(target, "https://"))
    {
      return Url::parse(target).without_fragment();
    }

    if (starts_with(target, "//"))
    {
      return Url::parse(base.scheme() + ":" + target).without_fragment();
    }

    if (target.front() == '/')
    {
      return base.origin() + target;
    }

    if (target.front() == '?')
    {
      return base.origin() + base.path() + target;
    }

    if (target.front() == '#')
    {
      return base.without_fragment();
    }

    const std::string mergedPath =
        normalize_path(directory_path(base.path()) + target);

    return base.origin() + mergedPath;
  }

  bool redirect_rewrites_method_to_get(
      int statusCode,
      std::string_view currentMethod) noexcept
  {
    if (statusCode == 303)
    {
      return !detail::ascii_iequals(currentMethod, "HEAD");
    }

    if ((statusCode == 301 || statusCode == 302) &&
        detail::ascii_iequals(currentMethod, "POST"))
    {
      return true;
    }

    return false;
  }

  bool redirect_preserves_method_and_body(
      int statusCode) noexcept
  {
    return statusCode == 307 || statusCode == 308;
  }

} // namespace vix::requests::http

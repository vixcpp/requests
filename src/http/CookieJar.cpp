/**
 *
 *  @file CookieJar.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Cookie storage helper implementation.
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

#include "http/CookieJar.hpp"
#include "detail/CaseInsensitive.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <utility>

namespace vix::requests::http
{
  namespace
  {
    [[nodiscard]] std::string trim(std::string_view value)
    {
      std::size_t start = 0;
      while (start < value.size() &&
             std::isspace(static_cast<unsigned char>(value[start])) != 0)
      {
        ++start;
      }

      std::size_t end = value.size();
      while (end > start &&
             std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
      {
        --end;
      }

      return std::string(value.substr(start, end - start));
    }

    [[nodiscard]] std::string lower(std::string_view value)
    {
      return detail::ascii_to_lower(value);
    }

    [[nodiscard]] bool starts_with(
        std::string_view value,
        std::string_view prefix) noexcept
    {
      return value.size() >= prefix.size() &&
             value.substr(0, prefix.size()) == prefix;
    }

    [[nodiscard]] bool ends_with(
        std::string_view value,
        std::string_view suffix) noexcept
    {
      return value.size() >= suffix.size() &&
             value.substr(value.size() - suffix.size()) == suffix;
    }

    [[nodiscard]] std::string default_cookie_path(const Url &url)
    {
      const std::string &path = url.path();

      if (path.empty() || path.front() != '/')
      {
        return "/";
      }

      const std::size_t slash = path.rfind('/');
      if (slash == std::string::npos || slash == 0)
      {
        return "/";
      }

      return path.substr(0, slash);
    }

    [[nodiscard]] bool domain_matches(
        std::string_view cookieDomain,
        std::string_view requestHost,
        bool hostOnly) noexcept
    {
      if (hostOnly)
      {
        return detail::ascii_iequals(cookieDomain, requestHost);
      }

      const std::string cookie = lower(cookieDomain);
      const std::string host = lower(requestHost);

      if (host == cookie)
      {
        return true;
      }

      return host.size() > cookie.size() &&
             ends_with(host, "." + cookie);
    }

    [[nodiscard]] bool path_matches(
        std::string_view cookiePath,
        std::string_view requestPath) noexcept
    {
      if (cookiePath.empty())
      {
        cookiePath = "/";
      }

      if (requestPath.empty())
      {
        requestPath = "/";
      }

      if (requestPath == cookiePath)
      {
        return true;
      }

      if (!starts_with(requestPath, cookiePath))
      {
        return false;
      }

      if (cookiePath.back() == '/')
      {
        return true;
      }

      return requestPath.size() > cookiePath.size() &&
             requestPath[cookiePath.size()] == '/';
    }

    [[nodiscard]] std::optional<long long> parse_integer(
        std::string_view value) noexcept
    {
      value = std::string_view(trim(value));

      if (value.empty())
      {
        return std::nullopt;
      }

      bool negative = false;
      std::size_t index = 0;

      if (value.front() == '-')
      {
        negative = true;
        index = 1;
      }
      else if (value.front() == '+')
      {
        index = 1;
      }

      if (index >= value.size())
      {
        return std::nullopt;
      }

      long long result = 0;

      for (; index < value.size(); ++index)
      {
        const unsigned char ch = static_cast<unsigned char>(value[index]);

        if (std::isdigit(ch) == 0)
        {
          return std::nullopt;
        }

        result = (result * 10LL) + static_cast<long long>(ch - '0');
      }

      return negative ? -result : result;
    }

    [[nodiscard]] bool valid_cookie_name(std::string_view name) noexcept
    {
      return detail::is_http_token(name);
    }

    [[nodiscard]] std::string normalize_domain(std::string_view domain)
    {
      std::string value = lower(trim(domain));

      while (!value.empty() && value.front() == '.')
      {
        value.erase(value.begin());
      }

      return value;
    }
  } // namespace

  bool Cookie::expired(Clock::time_point now) const noexcept
  {
    return expiresAt.has_value() && *expiresAt <= now;
  }

  bool Cookie::persistent() const noexcept
  {
    return expiresAt.has_value();
  }

  void CookieJar::store_from_response(
      const Url &url,
      const Headers &headers)
  {
    for (const auto &value : headers.get_all("Set-Cookie"))
    {
      store(url, value);
    }
  }

  void CookieJar::store(
      const Url &url,
      std::string_view setCookieHeader)
  {
    auto cookie = parse_set_cookie(url, setCookieHeader);
    if (!cookie.has_value())
    {
      return;
    }

    if (cookie->expired(Cookie::Clock::now()))
    {
      cookies_.erase(
          std::remove_if(
              cookies_.begin(),
              cookies_.end(),
              [&cookie](const Cookie &entry)
              {
                return detail::ascii_iequals(entry.name, cookie->name) &&
                       detail::ascii_iequals(entry.domain, cookie->domain) &&
                       entry.path == cookie->path;
              }),
          cookies_.end());
      return;
    }

    upsert(std::move(*cookie));
  }

  std::string CookieJar::cookie_header_for_url(const Url &url) const
  {
    const auto now = Cookie::Clock::now();
    std::string header;

    for (const auto &cookie : cookies_)
    {
      if (cookie.expired(now))
      {
        continue;
      }

      if (!cookie_matches_url(cookie, url))
      {
        continue;
      }

      if (!header.empty())
      {
        header += "; ";
      }

      header += cookie.name;
      header.push_back('=');
      header += cookie.value;
    }

    return header;
  }

  void CookieJar::apply_to(
      const Url &url,
      Headers &headers) const
  {
    if (headers.has("Cookie"))
    {
      return;
    }

    const std::string value = cookie_header_for_url(url);
    if (!value.empty())
    {
      headers.set("Cookie", value);
    }
  }

  void CookieJar::remove_expired()
  {
    const auto now = Cookie::Clock::now();

    cookies_.erase(
        std::remove_if(
            cookies_.begin(),
            cookies_.end(),
            [now](const Cookie &cookie)
            {
              return cookie.expired(now);
            }),
        cookies_.end());
  }

  void CookieJar::clear() noexcept
  {
    cookies_.clear();
  }

  bool CookieJar::empty() const noexcept
  {
    return cookies_.empty();
  }

  std::size_t CookieJar::size() const noexcept
  {
    return cookies_.size();
  }

  const CookieJar::Container &CookieJar::cookies() const noexcept
  {
    return cookies_;
  }

  void CookieJar::upsert(Cookie cookie)
  {
    for (Cookie &entry : cookies_)
    {
      if (detail::ascii_iequals(entry.name, cookie.name) &&
          detail::ascii_iequals(entry.domain, cookie.domain) &&
          entry.path == cookie.path)
      {
        entry = std::move(cookie);
        return;
      }
    }

    cookies_.push_back(std::move(cookie));
  }

  std::optional<Cookie> parse_set_cookie(
      const Url &url,
      std::string_view value)
  {
    const std::size_t firstSemicolon = value.find(';');
    const std::string_view pair =
        firstSemicolon == std::string_view::npos
            ? value
            : value.substr(0, firstSemicolon);

    const std::size_t equals = pair.find('=');
    if (equals == std::string_view::npos)
    {
      return std::nullopt;
    }

    Cookie cookie;
    cookie.name = trim(pair.substr(0, equals));
    cookie.value = trim(pair.substr(equals + 1));
    cookie.domain = lower(url.host());
    cookie.hostOnly = true;
    cookie.path = default_cookie_path(url);

    if (!valid_cookie_name(cookie.name))
    {
      return std::nullopt;
    }

    std::size_t cursor =
        firstSemicolon == std::string_view::npos
            ? value.size()
            : firstSemicolon + 1U;

    while (cursor < value.size())
    {
      const std::size_t next = value.find(';', cursor);
      const std::string attribute = trim(
          next == std::string_view::npos
              ? value.substr(cursor)
              : value.substr(cursor, next - cursor));

      if (!attribute.empty())
      {
        const std::size_t attrEquals = attribute.find('=');

        const std::string attrName = lower(
            attrEquals == std::string::npos
                ? attribute
                : attribute.substr(0, attrEquals));

        const std::string attrValue =
            attrEquals == std::string::npos
                ? std::string{}
                : trim(attribute.substr(attrEquals + 1));

        if (attrName == "domain")
        {
          const std::string normalized = normalize_domain(attrValue);

          if (!normalized.empty() &&
              domain_matches(normalized, url.host(), false))
          {
            cookie.domain = normalized;
            cookie.hostOnly = false;
          }
        }
        else if (attrName == "path")
        {
          if (!attrValue.empty() && attrValue.front() == '/')
          {
            cookie.path = attrValue;
          }
        }
        else if (attrName == "max-age")
        {
          const auto seconds = parse_integer(attrValue);

          if (seconds.has_value())
          {
            cookie.expiresAt =
                Cookie::Clock::now() + std::chrono::seconds(*seconds);
          }
        }
        else if (attrName == "secure")
        {
          cookie.secure = true;
        }
        else if (attrName == "httponly")
        {
          cookie.httpOnly = true;
        }
        else if (attrName == "samesite")
        {
          cookie.sameSite = attrValue;
        }
      }

      if (next == std::string_view::npos)
      {
        break;
      }

      cursor = next + 1U;
    }

    return cookie;
  }

  bool cookie_matches_url(
      const Cookie &cookie,
      const Url &url)
  {
    if (cookie.secure && !url.is_https())
    {
      return false;
    }

    if (!domain_matches(cookie.domain, url.host(), cookie.hostOnly))
    {
      return false;
    }

    if (!path_matches(cookie.path, url.path()))
    {
      return false;
    }

    return !cookie.expired(Cookie::Clock::now());
  }

} // namespace vix::requests::http

/**
 *
 *  @file Url.cpp
 *  @author Gaspard Kirira
 *
 *  @brief URL parser implementation for the Vix requests module.
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

#include <vix/requests/Url.hpp>
#include <vix/requests/Error.hpp>

#include "detail/CaseInsensitive.hpp"

#include <cctype>
#include <limits>
#include <sstream>

namespace vix::requests
{
  namespace
  {
    [[nodiscard]] bool is_scheme_char(unsigned char ch) noexcept
    {
      return std::isalnum(ch) != 0 ||
             ch == '+' ||
             ch == '-' ||
             ch == '.';
    }

    [[nodiscard]] std::string ascii_lower(std::string_view value)
    {
      std::string out;
      out.reserve(value.size());

      for (char raw_ch : value)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        out.push_back(static_cast<char>(std::tolower(ch)));
      }

      return out;
    }

    [[nodiscard]] std::uint16_t parse_port(std::string_view value)
    {
      if (value.empty())
      {
        throw InvalidUrlException("invalid URL: empty port");
      }

      unsigned int port = 0;

      for (char raw_ch : value)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (std::isdigit(ch) == 0)
        {
          throw InvalidUrlException("invalid URL: port must be numeric");
        }

        port = (port * 10U) + static_cast<unsigned int>(ch - '0');

        if (port > std::numeric_limits<std::uint16_t>::max())
        {
          throw InvalidUrlException("invalid URL: port out of range");
        }
      }

      if (port == 0)
      {
        throw InvalidUrlException("invalid URL: port must be greater than zero");
      }

      return static_cast<std::uint16_t>(port);
    }

    [[nodiscard]] std::size_t find_first_of_any(
        std::string_view value,
        std::string_view chars,
        std::size_t start) noexcept
    {
      for (std::size_t index = start; index < value.size(); ++index)
      {
        if (chars.find(value[index]) != std::string_view::npos)
        {
          return index;
        }
      }

      return std::string_view::npos;
    }

    [[nodiscard]] bool contains_ascii_space(std::string_view value) noexcept
    {
      for (char raw_ch : value)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (std::isspace(ch) != 0)
        {
          return true;
        }
      }

      return false;
    }

    [[nodiscard]] std::string host_for_authority(std::string_view host)
    {
      if (host.find(':') != std::string_view::npos &&
          !host.empty() &&
          host.front() != '[')
      {
        return "[" + std::string(host) + "]";
      }

      return std::string(host);
    }
  } // namespace

  Url Url::parse(std::string_view value)
  {
    if (value.empty())
    {
      throw InvalidUrlException("invalid URL: empty URL");
    }

    if (contains_ascii_space(value))
    {
      throw InvalidUrlException("invalid URL: whitespace is not allowed");
    }

    const std::size_t schemeEnd = value.find(':');
    if (schemeEnd == std::string_view::npos)
    {
      throw InvalidUrlException("invalid URL: missing scheme");
    }

    Url url;
    url.scheme_ = ascii_lower(value.substr(0, schemeEnd));
    validate_scheme(url.scheme_);

    std::size_t cursor = schemeEnd + 1;

    if (cursor + 1 >= value.size() ||
        value[cursor] != '/' ||
        value[cursor + 1] != '/')
    {
      throw InvalidUrlException("invalid URL: missing authority");
    }

    cursor += 2;

    const std::size_t authorityEnd = find_first_of_any(value, "/?#", cursor);
    const std::string_view authority =
        authorityEnd == std::string_view::npos
            ? value.substr(cursor)
            : value.substr(cursor, authorityEnd - cursor);

    if (authority.empty())
    {
      throw InvalidUrlException("invalid URL: empty authority");
    }

    if (authority.find('@') != std::string_view::npos)
    {
      throw InvalidUrlException("invalid URL: userinfo is not supported");
    }

    if (authority.front() == '[')
    {
      const std::size_t close = authority.find(']');
      if (close == std::string_view::npos)
      {
        throw InvalidUrlException("invalid URL: invalid IPv6 host");
      }

      url.host_ = std::string(authority.substr(1, close - 1));

      if (url.host_.empty())
      {
        throw InvalidUrlException("invalid URL: empty host");
      }

      if (close + 1 < authority.size())
      {
        if (authority[close + 1] != ':')
        {
          throw InvalidUrlException("invalid URL: invalid authority");
        }

        url.explicitPort_ = parse_port(authority.substr(close + 2));
      }
    }
    else
    {
      const std::size_t colon = authority.rfind(':');

      if (colon != std::string_view::npos)
      {
        url.host_ = std::string(authority.substr(0, colon));
        url.explicitPort_ = parse_port(authority.substr(colon + 1));
      }
      else
      {
        url.host_ = std::string(authority);
      }
    }

    if (url.host_.empty())
    {
      throw InvalidUrlException("invalid URL: empty host");
    }

    url.port_ = url.explicitPort_.value_or(
        default_port_for_scheme(url.scheme_));

    cursor = authorityEnd == std::string_view::npos
                 ? value.size()
                 : authorityEnd;

    if (cursor < value.size() && value[cursor] == '/')
    {
      const std::size_t pathEnd = find_first_of_any(value, "?#", cursor);
      url.path_ = std::string(
          pathEnd == std::string_view::npos
              ? value.substr(cursor)
              : value.substr(cursor, pathEnd - cursor));

      cursor = pathEnd == std::string_view::npos
                   ? value.size()
                   : pathEnd;
    }
    else
    {
      url.path_ = "/";
    }

    if (url.path_.empty())
    {
      url.path_ = "/";
    }

    if (cursor < value.size() && value[cursor] == '?')
    {
      const std::size_t queryStart = cursor + 1;
      const std::size_t fragmentStart = value.find('#', queryStart);

      url.query_ = std::string(
          fragmentStart == std::string_view::npos
              ? value.substr(queryStart)
              : value.substr(queryStart, fragmentStart - queryStart));

      cursor = fragmentStart == std::string_view::npos
                   ? value.size()
                   : fragmentStart;
    }

    if (cursor < value.size() && value[cursor] == '#')
    {
      url.fragment_ = std::string(value.substr(cursor + 1));
    }

    return url;
  }

  const std::string &Url::scheme() const noexcept
  {
    return scheme_;
  }

  const std::string &Url::host() const noexcept
  {
    return host_;
  }

  std::uint16_t Url::port() const noexcept
  {
    return port_;
  }

  std::optional<std::uint16_t> Url::explicit_port() const noexcept
  {
    return explicitPort_;
  }

  bool Url::has_explicit_port() const noexcept
  {
    return explicitPort_.has_value();
  }

  const std::string &Url::path() const noexcept
  {
    return path_;
  }

  const std::string &Url::query() const noexcept
  {
    return query_;
  }

  const std::string &Url::fragment() const noexcept
  {
    return fragment_;
  }

  bool Url::is_http() const noexcept
  {
    return detail::ascii_iequals(scheme_, "http");
  }

  bool Url::is_https() const noexcept
  {
    return detail::ascii_iequals(scheme_, "https");
  }

  bool Url::has_query() const noexcept
  {
    return !query_.empty();
  }

  std::string Url::authority() const
  {
    std::ostringstream oss;
    oss << host_for_authority(host_);

    if (explicitPort_.has_value())
    {
      oss << ':' << *explicitPort_;
    }

    return oss.str();
  }

  std::string Url::origin() const
  {
    return scheme_ + "://" + authority();
  }

  std::string Url::request_target() const
  {
    std::string target = path_.empty() ? "/" : path_;

    if (!query_.empty())
    {
      target.push_back('?');
      target += query_;
    }

    return target;
  }

  std::string Url::without_fragment() const
  {
    std::string out = origin();
    out += request_target();
    return out;
  }

  std::string Url::to_string() const
  {
    std::string out = without_fragment();

    if (!fragment_.empty())
    {
      out.push_back('#');
      out += fragment_;
    }

    return out;
  }

  Url Url::with_params(const Params &params) const
  {
    Url copy = *this;
    const std::string extra = params.to_query_string();

    if (extra.empty())
    {
      return copy;
    }

    if (!copy.query_.empty())
    {
      copy.query_.push_back('&');
    }

    copy.query_ += extra;
    return copy;
  }

  void Url::validate_scheme(std::string_view scheme)
  {
    if (scheme.empty())
    {
      throw InvalidUrlException("invalid URL: empty scheme");
    }

    const auto first = static_cast<unsigned char>(scheme.front());
    if (std::isalpha(first) == 0)
    {
      throw InvalidUrlException("invalid URL: scheme must start with a letter");
    }

    for (char raw_ch : scheme)
    {
      const auto ch = static_cast<unsigned char>(raw_ch);
      if (!is_scheme_char(ch))
      {
        throw InvalidUrlException("invalid URL: invalid scheme");
      }
    }
  }

  std::uint16_t Url::default_port_for_scheme(std::string_view scheme)
  {
    if (detail::ascii_iequals(scheme, "http"))
    {
      return 80;
    }

    if (detail::ascii_iequals(scheme, "https"))
    {
      return 443;
    }

    return 0;
  }

  Url parse_url(std::string_view value)
  {
    return Url::parse(value);
  }

  std::string append_query_string(
      std::string_view url,
      std::string_view query)
  {
    if (query.empty())
    {
      return std::string(url);
    }

    const std::size_t fragmentPos = url.find('#');

    const std::string_view beforeFragment =
        fragmentPos == std::string_view::npos
            ? url
            : url.substr(0, fragmentPos);

    const std::string_view fragment =
        fragmentPos == std::string_view::npos
            ? std::string_view{}
            : url.substr(fragmentPos);

    std::string out(beforeFragment);

    out.push_back(
        beforeFragment.find('?') == std::string_view::npos ? '?' : '&');

    out += query;
    out += fragment;

    return out;
  }

} // namespace vix::requests

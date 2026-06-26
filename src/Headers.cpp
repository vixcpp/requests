/**
 *
 *  @file Headers.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP header container implementation.
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

#include <vix/requests/Headers.hpp>
#include <vix/requests/Error.hpp>

#include "detail/CaseInsensitive.hpp"

#include <algorithm>

namespace vix::requests
{
  Headers::Headers(
      std::initializer_list<std::pair<std::string, std::string>> values)
  {
    for (const auto &entry : values)
    {
      set(entry.first, entry.second);
    }
  }

  void Headers::set(std::string_view name, std::string_view value)
  {
    validate_name(name);

    const auto index = find_index(name);
    if (index.has_value())
    {
      entries_[*index].value = detail::trim_ows(value);
      return;
    }

    append(name, value);
  }

  void Headers::append(std::string_view name, std::string_view value)
  {
    validate_name(name);

    entries_.push_back(Header{
        std::string(name),
        detail::trim_ows(value)});
  }

  std::optional<std::string> Headers::get(std::string_view name) const
  {
    const auto index = find_index(name);
    if (!index.has_value())
    {
      return std::nullopt;
    }

    return entries_[*index].value;
  }

  std::vector<std::string> Headers::get_all(std::string_view name) const
  {
    std::vector<std::string> values;

    for (const auto &entry : entries_)
    {
      if (detail::ascii_iequals(entry.name, name))
      {
        values.push_back(entry.value);
      }
    }

    return values;
  }

  bool Headers::has(std::string_view name) const noexcept
  {
    return find_index(name).has_value();
  }

  std::size_t Headers::remove(std::string_view name)
  {
    const auto oldSize = entries_.size();

    entries_.erase(
        std::remove_if(
            entries_.begin(),
            entries_.end(),
            [name](const Header &entry)
            {
              return detail::ascii_iequals(entry.name, name);
            }),
        entries_.end());

    return oldSize - entries_.size();
  }

  void Headers::clear() noexcept
  {
    entries_.clear();
  }

  bool Headers::empty() const noexcept
  {
    return entries_.empty();
  }

  std::size_t Headers::size() const noexcept
  {
    return entries_.size();
  }

  const Headers::Container &Headers::entries() const noexcept
  {
    return entries_;
  }

  Headers::iterator Headers::begin() noexcept
  {
    return entries_.begin();
  }

  Headers::iterator Headers::end() noexcept
  {
    return entries_.end();
  }

  Headers::const_iterator Headers::begin() const noexcept
  {
    return entries_.begin();
  }

  Headers::const_iterator Headers::end() const noexcept
  {
    return entries_.end();
  }

  Headers::const_iterator Headers::cbegin() const noexcept
  {
    return entries_.cbegin();
  }

  Headers::const_iterator Headers::cend() const noexcept
  {
    return entries_.cend();
  }

  std::optional<std::size_t> Headers::find_index(
      std::string_view name) const noexcept
  {
    for (std::size_t index = 0; index < entries_.size(); ++index)
    {
      if (detail::ascii_iequals(entries_[index].name, name))
      {
        return index;
      }
    }

    return std::nullopt;
  }

  void Headers::validate_name(std::string_view name)
  {
    if (!detail::is_http_token(name))
    {
      throw RequestException("invalid HTTP header name");
    }
  }

} // namespace vix::requests

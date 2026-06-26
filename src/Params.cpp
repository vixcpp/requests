/**
 *
 *  @file Params.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Query parameter container implementation.
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

#include <vix/requests/Params.hpp>

#include "detail/UrlEncode.hpp"

#include <algorithm>

namespace vix::requests
{
  Params::Params(
      std::initializer_list<std::pair<std::string, std::string>> values)
  {
    for (const auto &entry : values)
    {
      append(entry.first, entry.second);
    }
  }

  void Params::set(std::string_view name, std::string_view value)
  {
    const auto index = find_index(name);

    if (!index.has_value())
    {
      append(name, value);
      return;
    }

    entries_[*index].value = std::string(value);

    bool firstSeen = false;
    entries_.erase(
        std::remove_if(
            entries_.begin(),
            entries_.end(),
            [name, &firstSeen](const Param &entry)
            {
              if (entry.name != name)
              {
                return false;
              }

              if (!firstSeen)
              {
                firstSeen = true;
                return false;
              }

              return true;
            }),
        entries_.end());
  }

  void Params::append(std::string_view name, std::string_view value)
  {
    entries_.push_back(Param{
        std::string(name),
        std::string(value)});
  }

  std::optional<std::string> Params::get(std::string_view name) const
  {
    const auto index = find_index(name);
    if (!index.has_value())
    {
      return std::nullopt;
    }

    return entries_[*index].value;
  }

  std::vector<std::string> Params::get_all(std::string_view name) const
  {
    std::vector<std::string> values;

    for (const auto &entry : entries_)
    {
      if (entry.name == name)
      {
        values.push_back(entry.value);
      }
    }

    return values;
  }

  bool Params::has(std::string_view name) const noexcept
  {
    return find_index(name).has_value();
  }

  std::size_t Params::remove(std::string_view name)
  {
    const auto oldSize = entries_.size();

    entries_.erase(
        std::remove_if(
            entries_.begin(),
            entries_.end(),
            [name](const Param &entry)
            {
              return entry.name == name;
            }),
        entries_.end());

    return oldSize - entries_.size();
  }

  void Params::clear() noexcept
  {
    entries_.clear();
  }

  bool Params::empty() const noexcept
  {
    return entries_.empty();
  }

  std::size_t Params::size() const noexcept
  {
    return entries_.size();
  }

  std::string Params::to_query_string() const
  {
    return build_query_string(*this);
  }

  const Params::Container &Params::entries() const noexcept
  {
    return entries_;
  }

  Params::iterator Params::begin() noexcept
  {
    return entries_.begin();
  }

  Params::iterator Params::end() noexcept
  {
    return entries_.end();
  }

  Params::const_iterator Params::begin() const noexcept
  {
    return entries_.begin();
  }

  Params::const_iterator Params::end() const noexcept
  {
    return entries_.end();
  }

  Params::const_iterator Params::cbegin() const noexcept
  {
    return entries_.cbegin();
  }

  Params::const_iterator Params::cend() const noexcept
  {
    return entries_.cend();
  }

  std::optional<std::size_t> Params::find_index(
      std::string_view name) const noexcept
  {
    for (std::size_t index = 0; index < entries_.size(); ++index)
    {
      if (entries_[index].name == name)
      {
        return index;
      }
    }

    return std::nullopt;
  }

  std::string url_encode(std::string_view value)
  {
    return detail::url_encode_component(value);
  }

  std::string url_decode(std::string_view value)
  {
    return detail::url_decode_component(value);
  }

  std::string form_url_encode(std::string_view value)
  {
    return detail::form_url_encode_component(value);
  }

  std::string form_url_decode(std::string_view value)
  {
    return detail::form_url_decode_component(value);
  }

  std::string build_query_string(const Params &params)
  {
    std::string query;

    for (const auto &entry : params.entries())
    {
      if (!query.empty())
      {
        query.push_back('&');
      }

      query += detail::url_encode_component(entry.name);
      query.push_back('=');
      query += detail::url_encode_component(entry.value);
    }

    return query;
  }

} // namespace vix::requests

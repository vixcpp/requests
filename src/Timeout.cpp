/**
 *
 *  @file Timeout.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Timeout configuration implementation.
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

#include <vix/requests/Timeout.hpp>
#include <vix/requests/Error.hpp>

namespace vix::requests
{
  Timeout::Timeout(Duration value)
      : connect_(value),
        read_(value),
        total_(value)
  {
    validate(value);
  }

  Timeout::Timeout(
      Duration connect,
      Duration read,
      Duration total)
      : connect_(connect),
        read_(read),
        total_(total)
  {
    validate(connect_);
    validate(read_);
    validate(total_);
  }

  Timeout &Timeout::operator=(Duration value)
  {
    validate(value);
    connect_ = value;
    read_ = value;
    total_ = value;
    return *this;
  }

  Timeout Timeout::seconds(long seconds)
  {
    return Timeout(std::chrono::seconds(seconds));
  }

  Timeout Timeout::milliseconds(long milliseconds)
  {
    return Timeout(std::chrono::milliseconds(milliseconds));
  }

  Timeout Timeout::none()
  {
    return Timeout{};
  }

  Timeout::Duration Timeout::connect() const noexcept
  {
    return connect_;
  }

  Timeout::Duration Timeout::read() const noexcept
  {
    return read_;
  }

  Timeout::Duration Timeout::total() const noexcept
  {
    return total_;
  }

  void Timeout::set_connect(Duration value)
  {
    validate(value);
    connect_ = value;
  }

  void Timeout::set_read(Duration value)
  {
    validate(value);
    read_ = value;
  }

  void Timeout::set_total(Duration value)
  {
    validate(value);
    total_ = value;
  }

  bool Timeout::has_connect() const noexcept
  {
    return connect_.count() > 0;
  }

  bool Timeout::has_read() const noexcept
  {
    return read_.count() > 0;
  }

  bool Timeout::has_total() const noexcept
  {
    return total_.count() > 0;
  }

  bool Timeout::active() const noexcept
  {
    return has_connect() || has_read() || has_total();
  }

  void Timeout::validate(Duration value)
  {
    if (value.count() < 0)
    {
      throw RequestException("timeout value cannot be negative");
    }
  }

} // namespace vix::requests

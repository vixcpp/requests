/**
 *
 *  @file Body.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Request body helper implementation.
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

#include <vix/requests/Body.hpp>
#include "detail/UrlEncode.hpp"

#include <utility>

namespace vix::requests
{
  namespace
  {
    [[nodiscard]] std::string build_form_body(const Params &params)
    {
      std::string body;

      for (const auto &entry : params.entries())
      {
        if (!body.empty())
        {
          body.push_back('&');
        }

        body += detail::form_url_encode_component(entry.name);
        body.push_back('=');
        body += detail::form_url_encode_component(entry.value);
      }

      return body;
    }
  } // namespace

  Body::Body(
      BodyType type,
      std::string data,
      std::string contentType)
      : type_(type),
        data_(std::move(data)),
        contentType_(std::move(contentType))
  {
  }

  Body Body::binary(
      std::vector<unsigned char> data,
      std::string contentType)
  {
    std::string body;
    body.reserve(data.size());

    for (unsigned char byte : data)
    {
      body.push_back(static_cast<char>(byte));
    }

    return Body(
        data.empty() ? BodyType::Empty : BodyType::Binary,
        std::move(body),
        std::move(contentType));
  }

  BodyType Body::type() const noexcept
  {
    return type_;
  }

  const std::string &Body::data() const noexcept
  {
    return data_;
  }

  const std::string &Body::text() const noexcept
  {
    return data_;
  }

  std::vector<unsigned char> Body::bytes() const
  {
    return std::vector<unsigned char>(data_.begin(), data_.end());
  }

  const std::string &Body::content_type() const noexcept
  {
    return contentType_;
  }

  bool Body::empty() const noexcept
  {
    return data_.empty();
  }

  std::size_t Body::size() const noexcept
  {
    return data_.size();
  }

  bool Body::has_content_type() const noexcept
  {
    return !contentType_.empty();
  }

  Body raw_body(
      std::string_view data,
      std::string contentType)
  {
    return Body(
        data.empty() ? BodyType::Empty : BodyType::Raw,
        std::string(data),
        std::move(contentType));
  }

  Body binary_body(
      std::vector<unsigned char> data,
      std::string contentType)
  {
    return Body::binary(std::move(data), std::move(contentType));
  }

  Body json_body(std::string_view json)
  {
    return Body(
        json.empty() ? BodyType::Empty : BodyType::Json,
        std::string(json),
        "application/json");
  }

  Body form_body(const Params &params)
  {
    const std::string data = build_form_body(params);

    return Body(
        data.empty() ? BodyType::Empty : BodyType::Form,
        data,
        "application/x-www-form-urlencoded");
  }

  Body form_body(
      std::initializer_list<std::pair<std::string, std::string>> values)
  {
    return form_body(Params(values));
  }

  std::string_view to_string(BodyType type) noexcept
  {
    switch (type)
    {
    case BodyType::Empty:
      return "empty";

    case BodyType::Raw:
      return "raw";

    case BodyType::Json:
      return "json";

    case BodyType::Form:
      return "form";

    case BodyType::Binary:
      return "binary";
    }

    return "empty";
  }

} // namespace vix::requests

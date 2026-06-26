/**
 *
 *  @file HttpSerializer.cpp
 *  @author Gaspard Kirira
 *
 *  @brief HTTP request serializer implementation.
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

#include "http/HttpSerializer.hpp"
#include "detail/Base64.hpp"

#include <sstream>
#include <utility>

namespace vix::requests::http
{
  namespace
  {
    [[nodiscard]] Headers build_serialization_headers(const Request &request)
    {
      Headers headers = request.effective_headers();

      if (request.options().auth.configured() &&
          !headers.has("Authorization"))
      {
        headers.set(
            "Authorization",
            detail::make_basic_auth_value(
                request.options().auth.username,
                request.options().auth.password));
      }

      return headers;
    }
  } // namespace

  SerializedRequest serialize_request(const Request &request)
  {
    SerializedRequest serialized;
    serialized.head = serialize_request_head(request);

    if (request.has_body())
    {
      serialized.body = request.body().data();
    }

    serialized.data.reserve(serialized.head.size() + serialized.body.size());
    serialized.data += serialized.head;
    serialized.data += serialized.body;

    return serialized;
  }

  std::string serialize_request_head(const Request &request)
  {
    std::ostringstream out;

    out << request.method()
        << ' '
        << request.request_target()
        << " HTTP/1.1\r\n";

    out << serialize_headers(build_serialization_headers(request));
    out << "\r\n";

    return out.str();
  }

  std::string serialize_header_line(
      std::string_view name,
      std::string_view value)
  {
    std::string line;
    line.reserve(name.size() + 2U + value.size() + 2U);

    line.append(name);
    line.append(": ");
    line.append(value);
    line.append("\r\n");

    return line;
  }

  std::string serialize_headers(const Headers &headers)
  {
    std::string out;

    for (const auto &entry : headers.entries())
    {
      out += serialize_header_line(entry.name, entry.value);
    }

    return out;
  }

} // namespace vix::requests::http

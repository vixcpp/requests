/**
 *
 *  @file requests.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Public umbrella header for the Vix requests module.
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

#ifndef VIX_REQUESTS_REQUESTS_HPP
#define VIX_REQUESTS_REQUESTS_HPP

#include <vix/requests/Version.hpp>

#include <vix/requests/Error.hpp>
#include <vix/requests/Method.hpp>
#include <vix/requests/Headers.hpp>
#include <vix/requests/Params.hpp>
#include <vix/requests/Url.hpp>
#include <vix/requests/Body.hpp>
#include <vix/requests/Timeout.hpp>
#include <vix/requests/RequestOptions.hpp>
#include <vix/requests/Request.hpp>
#include <vix/requests/Response.hpp>
#include <vix/requests/Client.hpp>
#include <vix/requests/Session.hpp>

#include <vix/requests/transport/Transport.hpp>

/**
 * @brief Vix Requests public API.
 *
 * Main usage:
 *
 * @code
 * #include <vix/requests/requests.hpp>
 *
 * int main()
 * {
 *   auto response = vix::requests::get("http://example.com");
 *   response.raise_for_status();
 * }
 * @endcode
 *
 * Session usage:
 *
 * @code
 * vix::requests::Session session;
 * session.headers().set("Accept", "application/json");
 *
 * auto response = session.get("http://127.0.0.1:8080/api");
 * @endcode
 */

#endif // VIX_REQUESTS_REQUESTS_HPP

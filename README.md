# Vix Requests

`vix::requests` is a lightweight HTTP client module for Vix.cpp.

It provides a Python `requests`-like API for C++:

```cpp
#include <vix/requests/requests.hpp>

#include <iostream>

int main()
{
  auto response = vix::requests::get("https://example.com/");

  response.raise_for_status();

  std::cout << response.status_code() << "\n";
  std::cout << response.text() << "\n";
}
```

## Status

Version: `1.2.0`

This module is designed for Vix.cpp `v2.7.0`.

Current transport support:

| Protocol | Status                     |
| -------- | -------------------------- |
| HTTP     | Supported                  |
| HTTPS    | Supported with OpenSSL TLS |

HTTPS is implemented with OpenSSL TLS. TLS verification is enabled by default and can be disabled per request with:

```cpp
options.verify_tls = false;
```

This is useful for local development, self-signed certificates, or test fixtures.

The module does not use `curl`, `libcurl`, shell commands, `popen`, or temporary files.

## Features

- Simple free functions:
  - `get`
  - `post`
  - `put`
  - `patch`
  - `del`
  - `head`
  - `request`

- `Client` API

- `Session` API

- Headers with case-insensitive lookup

- Query params with URL encoding

- Raw body

- JSON body

- Form URL encoded body

- Binary body

- Basic auth

- Redirect handling

- Redirect loop detection

- Configurable max redirects

- Timeouts

- Response parsing

- `Content-Length`

- `Transfer-Encoding: chunked`

- Connection-close body

- `raise_for_status()`

- OpenSSL for HTTPS

- No curl

- No shell command execution

## Quick start

```cpp
#include <vix/requests/requests.hpp>

#include <iostream>

int main()
{
  try
  {
    const auto response =
        vix::requests::get("https://example.com/");

    std::cout << response.status_code() << "\n";
    std::cout << response.reason() << "\n";
    std::cout << response.text() << "\n";

    response.raise_for_status();

    return 0;
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request error: " << error.what() << "\n";
    return 1;
  }
}
```

Run it with Vix:

```bash
vix run examples/simple_get.cpp
```

## GET with params and headers

```cpp
#include <vix/requests/requests.hpp>

#include <chrono>
#include <iostream>

int main()
{
  vix::requests::RequestOptions options;

  options.headers.set("Accept", "application/json");
  options.params.set("page", "1");
  options.params.set("q", "vix requests");
  options.timeout = std::chrono::seconds(10);

  const auto response =
      vix::requests::get(
          "http://example.com/search",
          options);

  std::cout << response.status_code() << "\n";
  std::cout << response.text() << "\n";
}
```

Run it:

```bash
vix run examples/get_params.cpp
```

## POST JSON

```cpp
#include <vix/requests/requests.hpp>

#include <iostream>

int main()
{
  vix::requests::RequestOptions options;
  options.headers.set("Accept", "application/json");

  const auto response =
      vix::requests::post(
          "http://127.0.0.1:8080/api/items",
          vix::requests::json_body(R"({"name":"Vix"})"),
          options);

  std::cout << response.status_code() << "\n";
  std::cout << response.text() << "\n";
}
```

Run it:

```bash
vix run examples/post_json.cpp
```

## POST form

```cpp
#include <vix/requests/requests.hpp>

#include <iostream>

int main()
{
  const auto response =
      vix::requests::post(
          "http://127.0.0.1:8080/login",
          vix::requests::form_body({
              {"username", "gaspard"},
              {"project", "Vix Requests"}}));

  std::cout << response.status_code() << "\n";
  std::cout << response.text() << "\n";
}
```

Run it:

```bash
vix run examples/post_form.cpp
```

## Session

Use `Session` when multiple requests should share headers, params, auth, timeout, and cookies.

```cpp
#include <vix/requests/requests.hpp>

#include <chrono>
#include <iostream>

int main()
{
  vix::requests::Session session;

  session.headers().set("User-Agent", "vix-requests-example/1.0.0");
  session.headers().set("Accept", "application/json");

  session.params().set("token", "demo-token");
  session.timeout() = std::chrono::seconds(10);

  const auto profile =
      session.get("http://127.0.0.1:8080/api/profile");

  profile.raise_for_status();

  std::cout << profile.text() << "\n";

  const auto created =
      session.post(
          "http://127.0.0.1:8080/api/items",
          vix::requests::form_body({
              {"name", "Gaspard"},
              {"project", "Vix Requests"}}));

  created.raise_for_status();

  std::cout << created.text() << "\n";
}
```

Run it:

```bash
vix run examples/session.cpp
```

## Error handling

```cpp
#include <vix/requests/requests.hpp>

#include <iostream>

int main()
{
  try
  {
    auto response =
        vix::requests::get("http://example.com/missing");

    response.raise_for_status();
  }
  catch (const vix::requests::InvalidUrlException &error)
  {
    std::cerr << "invalid URL: " << error.what() << "\n";
  }
  catch (const vix::requests::UnsupportedProtocolException &error)
  {
    std::cerr << "unsupported protocol: " << error.what() << "\n";
  }
  catch (const vix::requests::TimeoutException &error)
  {
    std::cerr << "timeout: " << error.what() << "\n";
  }
  catch (const vix::requests::ConnectionException &error)
  {
    std::cerr << "connection error: " << error.what() << "\n";
  }
  catch (const vix::requests::TooManyRedirectsException &error)
  {
    std::cerr << "redirect error: " << error.what() << "\n";
  }
  catch (const vix::requests::HttpException &error)
  {
    std::cerr << "HTTP error: "
              << error.status_code()
              << " "
              << error.reason()
              << "\n";
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request error: " << error.what() << "\n";
  }
}
```

Run it:

```bash
vix run examples/error_handling.cpp
```

## Public API

Main include:

```cpp
#include <vix/requests/requests.hpp>
```

Main namespace:

```cpp
namespace vix::requests
```

Important public types:

```cpp
vix::requests::Client
vix::requests::Session
vix::requests::Request
vix::requests::Response
vix::requests::RequestOptions
vix::requests::Headers
vix::requests::Params
vix::requests::Body
vix::requests::Timeout
vix::requests::Url
```

## Request options

```cpp
vix::requests::RequestOptions options;

options.headers.set("Accept", "application/json");
options.params.set("page", "1");
options.timeout = std::chrono::seconds(10);

options.follow_redirects = true;
options.max_redirects = 10;

options.verify_tls = true;

options.set_basic_auth("username", "password");
options.set_user_agent("my-client/1.0");
```

## Response API

```cpp
response.status_code();
response.reason();
response.url();
response.headers();
response.header("Content-Type");
response.content_type();
response.content_length();
response.location();
response.text();
response.body();
response.bytes();
response.size();
response.empty();
response.ok();
response.is_redirect();
response.is_error();
response.raise_for_status();
response.elapsed();
```

## Build with Vix

Build the module:

```bash
vix build
```

Build in release mode:

```bash
vix build --preset release
```

Build all targets, including tests and examples:

```bash
vix build --build-target all
```

Clean and rebuild:

```bash
vix build --clean
```

Show detailed Vix output:

```bash
vix build -v
```

Show raw CMake/Ninja output when debugging:

```bash
vix build --cmake-verbose
```

## Build options

You can pass CMake options after `--`.

Build with tests enabled:

```bash
vix build --build-target all -- -DVIX_REQUESTS_BUILD_TESTS=ON
```

Build with examples enabled:

```bash
vix build --build-target all -- -DVIX_REQUESTS_BUILD_EXAMPLES=ON
```

Build with tests and examples enabled:

```bash
vix build --build-target all -- \
  -DVIX_REQUESTS_BUILD_TESTS=ON \
  -DVIX_REQUESTS_BUILD_EXAMPLES=ON
```

Release build with LTO:

```bash
vix build --preset release -- \
  -DVIX_REQUESTS_ENABLE_LTO=ON
```

## Tests

Build all targets first:

```bash
vix build --build-target all
```

Run all tests:

```bash
vix tests
```

Run tests with detailed Vix output:

```bash
vix tests -v
```

Run one test:

```bash
vix tests --test http_client_test
```

Short form:

```bash
vix tests -R http_client_test
```

Run the integration tests:

```bash
vix tests -R '^(http_client_test|redirect_test|timeout_test)$'
```

Run tests in release mode:

```bash
vix build --preset release --build-target all
vix tests --preset release
```

List available tests:

```bash
vix tests --list
```

Show raw internal runner output:

```bash
vix tests --raw
```

## Test files

Unit tests:

```txt
tests/unit/url_test.cpp
tests/unit/headers_test.cpp
tests/unit/params_test.cpp
tests/unit/body_test.cpp
tests/unit/response_parser_test.cpp
tests/unit/serializer_test.cpp
tests/unit/session_test.cpp
tests/unit/errors_test.cpp
```

Integration tests:

```txt
tests/integration/http_client_test.cpp
tests/integration/redirect_test.cpp
tests/integration/timeout_test.cpp
```

Tests do not require external internet access.

Integration tests use a local loopback HTTP server.

## Examples

```txt
examples/simple_get.cpp
examples/get_params.cpp
examples/post_json.cpp
examples/post_form.cpp
examples/session.cpp
examples/error_handling.cpp
```

Run examples with Vix:

```bash
vix run examples/simple_get.cpp
vix run examples/get_params.cpp
vix run examples/post_json.cpp
vix run examples/post_form.cpp
vix run examples/session.cpp
vix run examples/error_handling.cpp
```

## CMake target

When using the module from CMake directly:

```cmake
target_link_libraries(my_app PRIVATE vix::requests)
```

## CMake options

| Option                        | Default | Description                             |
| ----------------------------- | ------: | --------------------------------------- |
| `VIX_REQUESTS_BUILD_TESTS`    |    `ON` | Build tests in standalone mode          |
| `VIX_REQUESTS_BUILD_EXAMPLES` |    `ON` | Build examples in standalone mode       |
| `VIX_REQUESTS_ENABLE_LTO`     |   `OFF` | Enable LTO in release builds            |
| `VIX_REQUESTS_INSTALL`        |    `ON` | Enable standalone install/package rules |

Inside the Vix umbrella build, tests, examples, and standalone install rules are disabled by default unless explicitly enabled.

## Design

The module is organized as a real Vix module:

```txt
include/vix/requests/
  requests.hpp
  Client.hpp
  Session.hpp
  Request.hpp
  Response.hpp
  RequestOptions.hpp
  Headers.hpp
  Params.hpp
  Method.hpp
  Url.hpp
  Body.hpp
  Error.hpp
  Timeout.hpp
  Version.hpp

include/vix/requests/transport/
  Transport.hpp

src/
  Client.cpp
  Session.cpp
  Request.cpp
  Response.cpp
  RequestOptions.cpp
  Headers.cpp
  Params.cpp
  Method.cpp
  Url.cpp
  Body.cpp
  Error.cpp
  Timeout.cpp
  Version.cpp

src/http/
  HttpParser.hpp
  HttpParser.cpp
  HttpSerializer.hpp
  HttpSerializer.cpp
  RedirectPolicy.hpp
  RedirectPolicy.cpp
  CookieJar.hpp
  CookieJar.cpp

src/transport/
  Socket.hpp
  Socket.cpp
  Resolver.hpp
  Resolver.cpp
  TcpTransport.hpp
  TcpTransport.cpp
  TransportFactory.hpp
  TransportFactory.cpp

src/detail/
  Base64.hpp
  Base64.cpp
  UrlEncode.hpp
  UrlEncode.cpp
  CaseInsensitive.hpp
  CaseInsensitive.cpp
```

## Current limitations

- HTTPS is implemented with OpenSSL and supports SNI plus hostname verification by default.
- The current transport uses POSIX sockets.
- Windows transport is not implemented yet.
- No gzip or deflate decompression is advertised or faked.
- HTTP proxy support is not implemented yet.
- Connection reuse is prepared by design, but the current transport opens one connection per request.

## Recommended development workflow

```bash
vix build
vix tests
```

Before committing:

```bash
vix build --build-target all
vix tests
```

Before release:

```bash
vix build --preset release --build-target all
vix tests --preset release
```

## License

MIT

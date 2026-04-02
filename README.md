# vix/requests

HTTP for C++.

**Header-only. Simple. Expressive.**

---

## Download

https://vixcpp.com/registry/pkg/vix/requests

---

## Overview

`vix/requests` provides a simple and expressive API to perform HTTP requests in C++.

It supports:

- GET, POST, PUT, PATCH, DELETE
- headers
- query parameters
- JSON body
- form data
- sessions

It is designed to be:

- minimal
- predictable
- easy to use
- quick to integrate

---

## Why vix/requests?

Performing HTTP requests in C++ is often:

- verbose
- complex
- hard to read
- tightly coupled to low-level networking

This leads to:

- boilerplate code
- poor readability
- slow development

`vix/requests` provides:

- a clean API
- simple request construction
- readable code

---

## Installation

### Using Vix

```bash
vix add @vix/requests
vix install
```

### Manual

```bash
git clone https://github.com/vixcpp/requests.git
```

Add `include/` to your project.

---

## Requirements

- `curl` must be available on the system

---

## Basic Usage

```cpp
#include <vix/requests/requests.hpp>
#include <iostream>

int main()
{
  auto res = vix::requests::get("https://httpbin.org/get");

  std::cout << res.status_code << '\n';
  std::cout << res.text << '\n';
}
```

---

## Query Parameters

```cpp
vix::requests::RequestOptions options;
options.params = {
  {"q", "vix"},
  {"page", "1"}
};

auto res = vix::requests::get("https://httpbin.org/get", options);
```

---

## POST Request

```cpp
vix::requests::RequestOptions options;
options.body = "name=Gaspard&role=builder";
options.headers["Content-Type"] = "application/x-www-form-urlencoded";

auto res = vix::requests::post("https://httpbin.org/post", options);
```

---

## JSON Request

```cpp
vix::requests::RequestOptions options;
options.json = R"({
  "name": "Gaspard",
  "project": "Vix"
})";

auto res = vix::requests::post("https://httpbin.org/post", options);
```

---

## Custom Headers

```cpp
vix::requests::RequestOptions options;
options.headers["Authorization"] = "Bearer token";
options.headers["Accept"] = "application/json";

auto res = vix::requests::get("https://httpbin.org/get", options);
```

---

## Response

```cpp
if (res.ok())
{
  std::cout << res.text << '\n';
}

res.raise_for_status();
```

---

## Session

```cpp
vix::requests::Session session;

session.set_header("Accept", "application/json");
session.defaults().timeout_seconds = 10;

auto res = session.get("https://httpbin.org/get");
```

---

## Execution Model

- requests are executed via system `curl`
- response is fully buffered
- headers and body are parsed after execution

---

## Complexity

| Operation | Complexity |
|----------|-----------|
| request creation | O(1) |
| execution | O(n) |
| response parsing | O(n) |

---

## Design Philosophy

- minimal API
- explicit behavior
- no hidden magic
- composable options
- header-only simplicity

---

## Tests

```bash
vix build
vix test
```

---

## License

MIT License
Copyright (c) Gaspard Kirira


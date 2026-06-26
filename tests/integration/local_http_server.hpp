/**
 *
 *  @file local_http_server.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Small local HTTP test server for Vix Requests integration tests.
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

#ifndef VIX_REQUESTS_TESTS_INTEGRATION_LOCAL_HTTP_SERVER_HPP
#define VIX_REQUESTS_TESTS_INTEGRATION_LOCAL_HTTP_SERVER_HPP

#include <atomic>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace vix::requests::tests
{
  struct LocalHttpRequest
  {
    std::string method;
    std::string target;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string raw;
  };

  inline std::string local_http_reason_phrase(int statusCode)
  {
    switch (statusCode)
    {
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 204:
      return "No Content";
    case 302:
      return "Found";
    case 404:
      return "Not Found";
    case 500:
      return "Internal Server Error";
    default:
      return "OK";
    }
  }

  inline std::string local_http_response(
      int statusCode,
      std::string body = {},
      std::string contentType = "text/plain",
      std::string extraHeaders = {})
  {
    std::ostringstream out;

    out << "HTTP/1.1 "
        << statusCode
        << ' '
        << local_http_reason_phrase(statusCode)
        << "\r\n";

    if (!contentType.empty())
    {
      out << "Content-Type: " << contentType << "\r\n";
    }

    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n";

    if (!extraHeaders.empty())
    {
      out << extraHeaders;

      if (extraHeaders.size() < 2 ||
          extraHeaders.substr(extraHeaders.size() - 2) != "\r\n")
      {
        out << "\r\n";
      }
    }

    out << "\r\n";
    out << body;

    return out.str();
  }

  inline std::string local_http_redirect(std::string location)
  {
    return local_http_response(
        302,
        {},
        {},
        "Location: " + location + "\r\n");
  }

  class LocalHttpServer
  {
  public:
    using Handler = std::function<std::string(const LocalHttpRequest &)>;

    explicit LocalHttpServer(Handler handler)
        : handler_(std::move(handler))
    {
      start();
    }

    ~LocalHttpServer()
    {
      stop();
    }

    LocalHttpServer(const LocalHttpServer &) = delete;
    LocalHttpServer &operator=(const LocalHttpServer &) = delete;

    LocalHttpServer(LocalHttpServer &&) = delete;
    LocalHttpServer &operator=(LocalHttpServer &&) = delete;

    [[nodiscard]] std::uint16_t port() const noexcept
    {
      return port_;
    }

    [[nodiscard]] std::string url(std::string_view path = "/") const
    {
      std::string value = "http://127.0.0.1:";
      value += std::to_string(port_);
      value += path;
      return value;
    }

    void stop()
    {
      if (!running_.exchange(false))
      {
        return;
      }

      if (serverFd_ >= 0)
      {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
      }

      if (thread_.joinable())
      {
        thread_.join();
      }
    }

  private:
    Handler handler_;
    std::atomic<bool> running_{false};
    int serverFd_{-1};
    std::uint16_t port_{0};
    std::thread thread_;

    static std::string lowercase(std::string_view value)
    {
      std::string result;
      result.reserve(value.size());

      for (unsigned char ch : value)
      {
        result.push_back(static_cast<char>(std::tolower(ch)));
      }

      return result;
    }

    static std::string trim(std::string_view value)
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

    static std::size_t content_length_from_headers(
        const std::map<std::string, std::string> &headers)
    {
      const auto it = headers.find("content-length");

      if (it == headers.end())
      {
        return 0;
      }

      std::size_t value = 0;

      for (unsigned char ch : it->second)
      {
        if (std::isdigit(ch) == 0)
        {
          return 0;
        }

        value = (value * 10U) + static_cast<std::size_t>(ch - '0');
      }

      return value;
    }

    static LocalHttpRequest parse_request(const std::string &raw)
    {
      LocalHttpRequest request;
      request.raw = raw;

      const std::size_t headerEnd = raw.find("\r\n\r\n");
      if (headerEnd == std::string::npos)
      {
        throw std::runtime_error("invalid local test request");
      }

      const std::string head = raw.substr(0, headerEnd);
      const std::string body = raw.substr(headerEnd + 4U);

      std::istringstream stream(head);

      std::string requestLine;
      std::getline(stream, requestLine);

      if (!requestLine.empty() && requestLine.back() == '\r')
      {
        requestLine.pop_back();
      }

      std::istringstream requestLineStream(requestLine);
      std::string version;
      requestLineStream >> request.method;
      requestLineStream >> request.target;
      requestLineStream >> version;

      const std::size_t queryPos = request.target.find('?');

      if (queryPos == std::string::npos)
      {
        request.path = request.target;
      }
      else
      {
        request.path = request.target.substr(0, queryPos);
        request.query = request.target.substr(queryPos + 1U);
      }

      std::string line;
      while (std::getline(stream, line))
      {
        if (!line.empty() && line.back() == '\r')
        {
          line.pop_back();
        }

        if (line.empty())
        {
          break;
        }

        const std::size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
          continue;
        }

        request.headers[lowercase(line.substr(0, colon))] =
            trim(line.substr(colon + 1U));
      }

      request.body = body;
      return request;
    }

    static std::string read_request(int clientFd)
    {
      std::string data;
      char buffer[4096];

      while (data.find("\r\n\r\n") == std::string::npos)
      {
        const ssize_t received =
            ::recv(clientFd, buffer, sizeof(buffer), 0);

        if (received <= 0)
        {
          break;
        }

        data.append(buffer, static_cast<std::size_t>(received));
      }

      const std::size_t headerEnd = data.find("\r\n\r\n");
      if (headerEnd == std::string::npos)
      {
        return data;
      }

      const LocalHttpRequest partial = parse_request(data);
      const std::size_t contentLength =
          content_length_from_headers(partial.headers);

      const std::size_t bodyStart = headerEnd + 4U;

      while (data.size() < bodyStart + contentLength)
      {
        const ssize_t received =
            ::recv(clientFd, buffer, sizeof(buffer), 0);

        if (received <= 0)
        {
          break;
        }

        data.append(buffer, static_cast<std::size_t>(received));
      }

      return data;
    }

    static void send_all(int clientFd, std::string_view data)
    {
      std::size_t sentTotal = 0;

      while (sentTotal < data.size())
      {
        const ssize_t sent =
            ::send(
                clientFd,
                data.data() + sentTotal,
                data.size() - sentTotal,
                MSG_NOSIGNAL);

        if (sent <= 0)
        {
          return;
        }

        sentTotal += static_cast<std::size_t>(sent);
      }
    }

    void start()
    {
      serverFd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if (serverFd_ < 0)
      {
        throw std::runtime_error(std::strerror(errno));
      }

      int enabled = 1;
      static_cast<void>(
          ::setsockopt(
              serverFd_,
              SOL_SOCKET,
              SO_REUSEADDR,
              &enabled,
              sizeof(enabled)));

      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      address.sin_port = htons(0);

      if (::bind(
              serverFd_,
              reinterpret_cast<sockaddr *>(&address),
              sizeof(address)) != 0)
      {
        const std::string error = std::strerror(errno);
        ::close(serverFd_);
        serverFd_ = -1;
        throw std::runtime_error(error);
      }

      socklen_t addressLength = sizeof(address);

      if (::getsockname(
              serverFd_,
              reinterpret_cast<sockaddr *>(&address),
              &addressLength) != 0)
      {
        const std::string error = std::strerror(errno);
        ::close(serverFd_);
        serverFd_ = -1;
        throw std::runtime_error(error);
      }

      port_ = ntohs(address.sin_port);

      if (::listen(serverFd_, 16) != 0)
      {
        const std::string error = std::strerror(errno);
        ::close(serverFd_);
        serverFd_ = -1;
        throw std::runtime_error(error);
      }

      running_ = true;

      thread_ = std::thread(
          [this]
          {
            serve();
          });
    }

    void serve()
    {
      while (running_)
      {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);

        const int clientFd =
            ::accept(
                serverFd_,
                reinterpret_cast<sockaddr *>(&clientAddress),
                &clientLength);

        if (clientFd < 0)
        {
          if (running_)
          {
            continue;
          }

          break;
        }

        try
        {
          const std::string raw = read_request(clientFd);
          const LocalHttpRequest request = parse_request(raw);
          const std::string response = handler_(request);

          send_all(clientFd, response);
        }
        catch (...)
        {
          const std::string response =
              local_http_response(500, "local test server error");

          send_all(clientFd, response);
        }

        ::shutdown(clientFd, SHUT_RDWR);
        ::close(clientFd);
      }
    }
  };

} // namespace vix::requests::tests

#endif // VIX_REQUESTS_TESTS_INTEGRATION_LOCAL_HTTP_SERVER_HPP

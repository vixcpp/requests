#ifndef VIX_EXAMPLES_REQUESTS_EXAMPLE_SERVER_HPP
#define VIX_EXAMPLES_REQUESTS_EXAMPLE_SERVER_HPP

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

namespace vix_examples::requests
{
  struct ExampleRequest
  {
    std::string method;
    std::string target;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
  };

  [[nodiscard]] inline std::string reason_phrase(int statusCode)
  {
    switch (statusCode)
    {
    case 200:
      return "OK";
    case 302:
      return "Found";
    case 404:
      return "Not Found";
    default:
      return "OK";
    }
  }

  [[nodiscard]] inline std::string http_response(
      int statusCode,
      std::string body = {},
      std::string contentType = "application/json",
      std::string extraHeaders = {})
  {
    std::ostringstream out;
    out << "HTTP/1.1 " << statusCode << ' ' << reason_phrase(statusCode) << "\r\n";

    if (!contentType.empty())
    {
      out << "Content-Type: " << contentType << "\r\n";
    }

    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n";

    if (!extraHeaders.empty())
    {
      out << extraHeaders;
      if (extraHeaders.size() < 2 || extraHeaders.substr(extraHeaders.size() - 2) != "\r\n")
      {
        out << "\r\n";
      }
    }

    out << "\r\n" << body;
    return out.str();
  }

  class ExampleHttpServer
  {
  public:
    ExampleHttpServer()
    {
      start();
    }

    ~ExampleHttpServer()
    {
      stop();
    }

    ExampleHttpServer(const ExampleHttpServer &) = delete;
    ExampleHttpServer &operator=(const ExampleHttpServer &) = delete;

    [[nodiscard]] std::string url(std::string_view path = "") const
    {
      std::string value = "http://127.0.0.1:";
      value += std::to_string(port_);
      value += path;
      return value;
    }

    [[nodiscard]] std::string base_url() const
    {
      return url();
    }

  private:
    std::atomic<bool> running_{false};
    int serverFd_{-1};
    std::uint16_t port_{0};
    std::thread thread_;

    [[nodiscard]] static std::string lowercase(std::string_view value)
    {
      std::string result;
      result.reserve(value.size());
      for (char raw_ch : value)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        result.push_back(static_cast<char>(std::tolower(ch)));
      }
      return result;
    }

    [[nodiscard]] static std::string trim(std::string_view value)
    {
      std::size_t start = 0;
      while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
      {
        ++start;
      }

      std::size_t end = value.size();
      while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
      {
        --end;
      }

      return std::string(value.substr(start, end - start));
    }

    [[nodiscard]] static std::size_t content_length(const std::map<std::string, std::string> &headers)
    {
      const auto it = headers.find("content-length");
      if (it == headers.end())
      {
        return 0;
      }

      std::size_t value = 0;
      for (char raw_ch : it->second)
      {
        const auto ch = static_cast<unsigned char>(raw_ch);
        if (std::isdigit(ch) == 0)
        {
          return 0;
        }
        value = (value * 10U) + static_cast<std::size_t>(ch - '0');
      }
      return value;
    }

    [[nodiscard]] static ExampleRequest parse_request(const std::string &raw)
    {
      ExampleRequest request;
      const std::size_t headerEnd = raw.find("\r\n\r\n");
      if (headerEnd == std::string::npos)
      {
        throw std::runtime_error("invalid example request");
      }

      std::istringstream head(raw.substr(0, headerEnd));
      std::string requestLine;
      std::getline(head, requestLine);
      if (!requestLine.empty() && requestLine.back() == '\r')
      {
        requestLine.pop_back();
      }

      std::istringstream requestLineStream(requestLine);
      std::string version;
      requestLineStream >> request.method >> request.target >> version;

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
      while (std::getline(head, line))
      {
        if (!line.empty() && line.back() == '\r')
        {
          line.pop_back();
        }

        const std::size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
          request.headers[lowercase(line.substr(0, colon))] = trim(line.substr(colon + 1U));
        }
      }

      request.body = raw.substr(headerEnd + 4U);
      return request;
    }

    [[nodiscard]] static std::string read_request(int clientFd)
    {
      std::string data;
      char buffer[4096];

      while (data.find("\r\n\r\n") == std::string::npos)
      {
        const ssize_t received = ::recv(clientFd, buffer, sizeof(buffer), 0);
        if (received <= 0)
        {
          return data;
        }
        data.append(buffer, static_cast<std::size_t>(received));
      }

      const std::size_t headerEnd = data.find("\r\n\r\n");
      const ExampleRequest partial = parse_request(data);
      const std::size_t expectedSize = headerEnd + 4U + content_length(partial.headers);

      while (data.size() < expectedSize)
      {
        const ssize_t received = ::recv(clientFd, buffer, sizeof(buffer), 0);
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
        const ssize_t sent = ::send(
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

    [[nodiscard]] std::string handle(const ExampleRequest &request) const
    {
      if (request.path == "/get" || request.path == "/search")
      {
        return http_response(
            200,
            "{\"ok\":true,\"method\":\"GET\",\"path\":\"" + request.path +
                "\",\"query\":\"" + request.query + "\"}");
      }

      if (request.path == "/post" || request.path == "/login" || request.path == "/api/items")
      {
        return http_response(
            200,
            "{\"ok\":true,\"method\":\"POST\",\"body_size\":" + std::to_string(request.body.size()) + "}");
      }

      if (request.path == "/api/profile")
      {
        return http_response(200, "{\"name\":\"Demo User\",\"project\":\"Vix Requests\"}");
      }

      if (request.path == "/cookies/set/session/demo-token")
      {
        return http_response(
            302,
            {},
            {},
            "Location: /cookies\r\nSet-Cookie: session=demo-token; Path=/\r\n");
      }

      if (request.path == "/cookies")
      {
        return http_response(200, "{\"cookies\":{\"session\":\"demo-token\"}}");
      }

      if (request.path == "/bearer")
      {
        const bool hasAuth = request.headers.find("authorization") != request.headers.end();
        return http_response(hasAuth ? 200 : 401, hasAuth ? "{\"authenticated\":true}" : "{\"authenticated\":false}");
      }

      if (request.path == "/bytes/1024")
      {
        return http_response(200, std::string(1024, 'x'), "application/octet-stream");
      }

      if (request.path == "/delay/1")
      {
        return http_response(200, "{\"ok\":true,\"delay\":0}");
      }

      if (request.path == "/status/404" || request.path == "/missing")
      {
        return http_response(404, "{\"error\":\"not found\"}");
      }

      return http_response(404, "{\"error\":\"not found\"}");
    }

    void start()
    {
      serverFd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (serverFd_ < 0)
      {
        throw std::runtime_error(std::strerror(errno));
      }

      int enabled = 1;
      static_cast<void>(::setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)));

      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      address.sin_port = htons(0);

      if (::bind(serverFd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
      {
        const std::string error = std::strerror(errno);
        ::close(serverFd_);
        serverFd_ = -1;
        throw std::runtime_error(error);
      }

      socklen_t addressLength = sizeof(address);
      if (::getsockname(serverFd_, reinterpret_cast<sockaddr *>(&address), &addressLength) != 0)
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
      thread_ = std::thread([this] { serve(); });
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

    void serve()
    {
      while (running_)
      {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);
        const int clientFd = ::accept(serverFd_, reinterpret_cast<sockaddr *>(&clientAddress), &clientLength);

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
          const ExampleRequest request = parse_request(raw);
          send_all(clientFd, handle(request));
        }
        catch (...)
        {
          send_all(clientFd, http_response(500, "{\"error\":\"example server error\"}"));
        }

        ::shutdown(clientFd, SHUT_RDWR);
        ::close(clientFd);
      }
    }
  };
}

#endif // VIX_EXAMPLES_REQUESTS_EXAMPLE_SERVER_HPP

/**
 * @file requests.hpp
 * @brief HTTP utilities for C++
 *
 * @version 0.1.0
 * @author Gaspard Kirira
 * @copyright (c) 2026 Gaspard Kirira
 * @license MIT
 *
 * @details
 * This header provides a simple and expressive HTTP API for C++.
 *
 * Features:
 * - GET, POST, PUT, PATCH, DELETE, HEAD
 * - headers
 * - query parameters
 * - request body
 * - JSON body
 * - form urlencoded body
 * - redirects
 * - timeout
 * - basic auth
 * - reusable session object
 *
 * Example:
 * @code
 * #include <iostream>
 * #include <vix/requests/requests.hpp>
 *
 * int main()
 * {
 *   auto res = vix::requests::get("https://httpbin.org/get");
 *
 *   std::cout << res.status_code << '\n';
 *   std::cout << res.text << '\n';
 * }
 * @endcode
 */

#ifndef VIX_REQUESTS_REQUESTS_HPP
#define VIX_REQUESTS_REQUESTS_HPP

#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::requests
{
  using Headers = std::map<std::string, std::string>;
  using Params = std::map<std::string, std::string>;

  /**
   * @brief Base exception for request-related failures.
   */
  class RequestException : public std::runtime_error
  {
  public:
    explicit RequestException(const std::string &message) : std::runtime_error(message) {}
  };

  /**
   * @brief Raised when the curl executable cannot be executed correctly.
   */
  class TransportException : public RequestException
  {
  public:
    explicit TransportException(const std::string &message) : RequestException(message) {}
  };

  /**
   * @brief Raised when a request times out.
   */
  class TimeoutException : public RequestException
  {
  public:
    explicit TimeoutException(const std::string &message) : RequestException(message) {}
  };

  /**
   * @brief Raised when the response status code indicates an HTTP error.
   */
  class HttpException : public RequestException
  {
  public:
    explicit HttpException(const std::string &message) : RequestException(message) {}
  };

  /**
   * @brief HTTP response container.
   */
  struct Response
  {
    /**
     * @brief Final request URL.
     */
    std::string url;

    /**
     * @brief HTTP status code.
     */
    int status_code = 0;

    /**
     * @brief Reason phrase if available.
     */
    std::string reason;

    /**
     * @brief Response headers.
     */
    Headers headers;

    /**
     * @brief Response body as text.
     */
    std::string text;

    /**
     * @brief Returns true when the status code is in the 2xx range.
     */
    [[nodiscard]] bool ok() const noexcept
    {
      return status_code >= 200 && status_code < 300;
    }

    /**
     * @brief Returns the response body bytes.
     */
    [[nodiscard]] std::vector<unsigned char> bytes() const
    {
      return std::vector<unsigned char>(text.begin(), text.end());
    }

    /**
     * @brief Throws an exception if the response is not successful.
     */
    void raise_for_status() const
    {
      if (!ok())
      {
        std::ostringstream oss;
        oss << "HTTP request failed with status " << status_code;
        if (!reason.empty())
        {
          oss << " (" << reason << ")";
        }
        throw HttpException(oss.str());
      }
    }

    /**
     * @brief Returns the header value for the given key if present.
     */
    [[nodiscard]] std::optional<std::string> header(const std::string &key) const
    {
      auto it = headers.find(key);
      if (it != headers.end())
      {
        return it->second;
      }

      for (const auto &[k, v] : headers)
      {
        if (to_lower(k) == to_lower(key))
        {
          return v;
        }
      }

      return std::nullopt;
    }

  private:
    static std::string to_lower(std::string value)
    {
      for (char &ch : value)
      {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
      }
      return value;
    }
  };

  /**
   * @brief Request options.
   */
  struct RequestOptions
  {
    /**
     * @brief Extra headers to send.
     */
    Headers headers;

    /**
     * @brief Query parameters appended to the URL.
     */
    Params params;

    /**
     * @brief Raw request body.
     */
    std::string body;

    /**
     * @brief JSON body.
     *
     * If non-empty, Content-Type: application/json is automatically added
     * unless already present in headers.
     */
    std::string json;

    /**
     * @brief Form fields encoded as application/x-www-form-urlencoded.
     *
     * If non-empty, Content-Type: application/x-www-form-urlencoded is
     * automatically added unless already present in headers.
     */
    Params form;

    /**
     * @brief Username for basic authentication.
     */
    std::string username;

    /**
     * @brief Password for basic authentication.
     */
    std::string password;

    /**
     * @brief Timeout in seconds.
     *
     * Value <= 0 means no explicit timeout flag.
     */
    long timeout_seconds = 0;

    /**
     * @brief Follow redirects automatically.
     */
    bool follow_redirects = true;

    /**
     * @brief Verify TLS certificates.
     */
    bool verify_tls = true;

    /**
     * @brief User-Agent value.
     */
    std::string user_agent = "vix-requests/0.1.0";

    /**
     * @brief Accept compressed responses.
     */
    bool compressed = true;
  };

  namespace detail
  {
    inline std::string trim(const std::string &value)
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

      return value.substr(start, end - start);
    }

    inline std::string to_lower(std::string value)
    {
      for (char &ch : value)
      {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
      }
      return value;
    }

    inline bool has_header(const Headers &headers, const std::string &name)
    {
      const std::string target = to_lower(name);

      for (const auto &[k, _] : headers)
      {
        if (to_lower(k) == target)
        {
          return true;
        }
      }

      return false;
    }

    inline std::string shell_escape(const std::string &value)
    {
#ifdef _WIN32
      std::string escaped = "\"";
      for (char ch : value)
      {
        if (ch == '"')
        {
          escaped += "\\\"";
        }
        else
        {
          escaped += ch;
        }
      }
      escaped += "\"";
      return escaped;
#else
      std::string escaped = "'";
      for (char ch : value)
      {
        if (ch == '\'')
        {
          escaped += "'\\''";
        }
        else
        {
          escaped += ch;
        }
      }
      escaped += "'";
      return escaped;
#endif
    }

    inline std::string read_file_as_string(const std::filesystem::path &path)
    {
      std::ifstream in(path, std::ios::binary);
      if (!in)
      {
        throw TransportException("failed to open file: " + path.string());
      }

      std::ostringstream oss;
      oss << in.rdbuf();
      return oss.str();
    }

    inline void write_file_string(const std::filesystem::path &path, const std::string &content)
    {
      std::ofstream out(path, std::ios::binary);
      if (!out)
      {
        throw TransportException("failed to write file: " + path.string());
      }
      out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    inline std::filesystem::path make_temp_path(const std::string &prefix,
                                                const std::string &suffix)
    {
      const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
      const auto dir = std::filesystem::temp_directory_path();

      std::ostringstream oss;
      oss << prefix << "_" << static_cast<unsigned long long>(now)
          << "_" << std::rand() << suffix;

      return dir / oss.str();
    }

    struct TempFiles
    {
      std::filesystem::path body_path;
      std::filesystem::path header_path;
      std::filesystem::path request_body_path;
    };

    struct ProcessResult
    {
      int exit_code = 0;
      std::string stdout_text;
    };

    inline ProcessResult run_capture_stdout(const std::string &command)
    {
#ifdef _WIN32
      FILE *pipe = _popen(command.c_str(), "r");
#else
      FILE *pipe = popen(command.c_str(), "r");
#endif
      if (pipe == nullptr)
      {
        throw TransportException("failed to execute curl command");
      }

      std::array<char, 256> buffer{};
      std::string output;

      while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
      {
        output += buffer.data();
      }

#ifdef _WIN32
      const int raw = _pclose(pipe);
      const int exit_code = raw;
#else
      const int raw = pclose(pipe);
      int exit_code = raw;
      if (raw != -1)
      {
        exit_code = WEXITSTATUS(raw);
      }
#endif

      return {exit_code, output};
    }

    inline std::string status_reason_from_status_line(const std::string &line)
    {
      std::istringstream iss(line);
      std::string http_version;
      std::string status_code;
      std::string reason;

      iss >> http_version >> status_code;
      std::getline(iss, reason);

      return trim(reason);
    }

    inline Response parse_response(const std::string &url,
                                   const std::string &status_text,
                                   const std::string &headers_text,
                                   const std::string &body_text)
    {
      Response response;
      response.url = url;
      response.text = body_text;

      const std::string trimmed_status = trim(status_text);
      if (!trimmed_status.empty())
      {
        response.status_code = std::atoi(trimmed_status.c_str());
      }

      std::istringstream stream(headers_text);
      std::string line;
      Headers current_headers;
      std::string current_reason;

      while (std::getline(stream, line))
      {
        if (!line.empty() && line.back() == '\r')
        {
          line.pop_back();
        }

        if (line.empty())
        {
          continue;
        }

        if (line.rfind("HTTP/", 0) == 0)
        {
          current_headers.clear();
          current_reason = status_reason_from_status_line(line);
          continue;
        }

        const auto pos = line.find(':');
        if (pos == std::string::npos)
        {
          continue;
        }

        const std::string key = trim(line.substr(0, pos));
        const std::string value = trim(line.substr(pos + 1));
        current_headers[key] = value;
      }

      response.headers = std::move(current_headers);
      response.reason = std::move(current_reason);

      return response;
    }

    inline std::string join_header_argument(const std::string &key, const std::string &value)
    {
      return key + ": " + value;
    }

    inline bool has_query_string(const std::string &url)
    {
      return url.find('?') != std::string::npos;
    }

    inline std::string remove_trailing_newlines(std::string value)
    {
      while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
      {
        value.pop_back();
      }
      return value;
    }
  } // namespace detail

  /**
   * @brief Percent-encodes a string for URL usage.
   */
  inline std::string url_encode(std::string_view value)
  {
    static constexpr char hex[] = "0123456789ABCDEF";

    std::string encoded;
    encoded.reserve(value.size() * 3);

    for (unsigned char ch : value)
    {
      const bool unreserved =
          std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch == '.' || ch == '~';

      if (unreserved)
      {
        encoded.push_back(static_cast<char>(ch));
      }
      else
      {
        encoded.push_back('%');
        encoded.push_back(hex[(ch >> 4) & 0x0F]);
        encoded.push_back(hex[ch & 0x0F]);
      }
    }

    return encoded;
  }

  /**
   * @brief Builds a query string from parameters.
   */
  inline std::string build_query_string(const Params &params)
  {
    std::string query;
    bool first = true;

    for (const auto &[key, value] : params)
    {
      if (!first)
      {
        query += '&';
      }
      first = false;
      query += url_encode(key);
      query += '=';
      query += url_encode(value);
    }

    return query;
  }

  /**
   * @brief Appends query parameters to a URL.
   */
  inline std::string with_params(const std::string &url, const Params &params)
  {
    if (params.empty())
    {
      return url;
    }

    const std::string query = build_query_string(params);
    if (query.empty())
    {
      return url;
    }

    std::string full = url;
    full += detail::has_query_string(url) ? '&' : '?';
    full += query;
    return full;
  }

  /**
   * @brief Performs a raw HTTP request.
   *
   * @param method HTTP method.
   * @param url Target URL.
   * @param options Request options.
   * @return Response HTTP response.
   */
  inline Response request(const std::string &method,
                          const std::string &url,
                          RequestOptions options = {})
  {
    const std::string final_url = with_params(url, options.params);

    detail::TempFiles temp;
    temp.body_path = detail::make_temp_path("vix_requests_body", ".tmp");
    temp.header_path = detail::make_temp_path("vix_requests_headers", ".tmp");
    temp.request_body_path = detail::make_temp_path("vix_requests_request_body", ".tmp");

    struct TempCleanup
    {
      detail::TempFiles files;

      ~TempCleanup()
      {
        std::error_code ec;
        if (!files.body_path.empty())
        {
          std::filesystem::remove(files.body_path, ec);
        }
        if (!files.header_path.empty())
        {
          std::filesystem::remove(files.header_path, ec);
        }
        if (!files.request_body_path.empty())
        {
          std::filesystem::remove(files.request_body_path, ec);
        }
      }
    } cleanup{temp};

    Headers final_headers = options.headers;

    std::string request_body;
    if (!options.json.empty())
    {
      request_body = options.json;
      if (!detail::has_header(final_headers, "Content-Type"))
      {
        final_headers["Content-Type"] = "application/json";
      }
    }
    else if (!options.form.empty())
    {
      request_body = build_query_string(options.form);
      if (!detail::has_header(final_headers, "Content-Type"))
      {
        final_headers["Content-Type"] = "application/x-www-form-urlencoded";
      }
    }
    else
    {
      request_body = options.body;
    }

    std::ostringstream cmd;
    cmd << "curl -sS";
    cmd << " -X " << detail::shell_escape(method);

    if (options.follow_redirects)
    {
      cmd << " -L";
    }

    if (!options.verify_tls)
    {
      cmd << " -k";
    }

    if (options.compressed)
    {
      cmd << " --compressed";
    }

    if (!options.user_agent.empty())
    {
      cmd << " -A " << detail::shell_escape(options.user_agent);
    }

    if (options.timeout_seconds > 0)
    {
      cmd << " --max-time " << options.timeout_seconds;
    }

    if (!options.username.empty() || !options.password.empty())
    {
      cmd << " -u "
          << detail::shell_escape(options.username + ":" + options.password);
    }

    for (const auto &[key, value] : final_headers)
    {
      cmd << " -H " << detail::shell_escape(detail::join_header_argument(key, value));
    }

    if (!request_body.empty())
    {
      detail::write_file_string(temp.request_body_path, request_body);
      cmd << " --data-binary @" << detail::shell_escape(temp.request_body_path.string());
    }

    cmd << " -D " << detail::shell_escape(temp.header_path.string());
    cmd << " -o " << detail::shell_escape(temp.body_path.string());
    cmd << " -w " << detail::shell_escape("%{http_code}");
    cmd << " " << detail::shell_escape(final_url);

    const detail::ProcessResult proc = detail::run_capture_stdout(cmd.str());

    if (proc.exit_code != 0)
    {
      std::ostringstream oss;
      oss << "curl command failed with exit code " << proc.exit_code;

      if (proc.exit_code == 28)
      {
        throw TimeoutException(oss.str());
      }

      throw TransportException(oss.str());
    }

    const std::string status_text = detail::remove_trailing_newlines(proc.stdout_text);
    const std::string headers_text = std::filesystem::exists(temp.header_path)
                                         ? detail::read_file_as_string(temp.header_path)
                                         : std::string{};
    const std::string body_text = std::filesystem::exists(temp.body_path)
                                      ? detail::read_file_as_string(temp.body_path)
                                      : std::string{};

    return detail::parse_response(final_url, status_text, headers_text, body_text);
  }

  /**
   * @brief Sends a GET request.
   */
  inline Response get(const std::string &url, RequestOptions options = {})
  {
    return request("GET", url, std::move(options));
  }

  /**
   * @brief Sends a POST request.
   */
  inline Response post(const std::string &url, RequestOptions options = {})
  {
    return request("POST", url, std::move(options));
  }

  /**
   * @brief Sends a PUT request.
   */
  inline Response put(const std::string &url, RequestOptions options = {})
  {
    return request("PUT", url, std::move(options));
  }

  /**
   * @brief Sends a PATCH request.
   */
  inline Response patch(const std::string &url, RequestOptions options = {})
  {
    return request("PATCH", url, std::move(options));
  }

  /**
   * @brief Sends a DELETE request.
   */
  inline Response del(const std::string &url, RequestOptions options = {})
  {
    return request("DELETE", url, std::move(options));
  }

  /**
   * @brief Sends a HEAD request.
   */
  inline Response head(const std::string &url, RequestOptions options = {})
  {
    return request("HEAD", url, std::move(options));
  }

  /**
   * @brief Reusable HTTP session with default options.
   */
  class Session
  {
  public:
    /**
     * @brief Returns mutable default options.
     */
    [[nodiscard]] RequestOptions &defaults() noexcept
    {
      return defaults_;
    }

    /**
     * @brief Returns immutable default options.
     */
    [[nodiscard]] const RequestOptions &defaults() const noexcept
    {
      return defaults_;
    }

    /**
     * @brief Sets a default header.
     */
    void set_header(const std::string &key, const std::string &value)
    {
      defaults_.headers[key] = value;
    }

    /**
     * @brief Removes a default header.
     */
    void remove_header(const std::string &key)
    {
      auto it = defaults_.headers.find(key);
      if (it != defaults_.headers.end())
      {
        defaults_.headers.erase(it);
        return;
      }

      for (auto iter = defaults_.headers.begin(); iter != defaults_.headers.end(); ++iter)
      {
        if (detail::to_lower(iter->first) == detail::to_lower(key))
        {
          defaults_.headers.erase(iter);
          return;
        }
      }
    }

    /**
     * @brief Sends a GET request using session defaults.
     */
    Response get(const std::string &url, RequestOptions options = {}) const
    {
      return request("GET", url, merge(options));
    }

    /**
     * @brief Sends a POST request using session defaults.
     */
    Response post(const std::string &url, RequestOptions options = {}) const
    {
      return request("POST", url, merge(options));
    }

    /**
     * @brief Sends a PUT request using session defaults.
     */
    Response put(const std::string &url, RequestOptions options = {}) const
    {
      return request("PUT", url, merge(options));
    }

    /**
     * @brief Sends a PATCH request using session defaults.
     */
    Response patch(const std::string &url, RequestOptions options = {}) const
    {
      return request("PATCH", url, merge(options));
    }

    /**
     * @brief Sends a DELETE request using session defaults.
     */
    Response del(const std::string &url, RequestOptions options = {}) const
    {
      return request("DELETE", url, merge(options));
    }

    /**
     * @brief Sends a HEAD request using session defaults.
     */
    Response head(const std::string &url, RequestOptions options = {}) const
    {
      return request("HEAD", url, merge(options));
    }

    /**
     * @brief Sends a request using session defaults.
     */
    Response request(const std::string &method,
                     const std::string &url,
                     RequestOptions options = {}) const
    {
      return vix::requests::request(method, url, merge(options));
    }

  private:
    RequestOptions defaults_{};

    [[nodiscard]] RequestOptions merge(RequestOptions options) const
    {
      RequestOptions merged = defaults_;

      for (const auto &[k, v] : options.headers)
      {
        merged.headers[k] = v;
      }

      for (const auto &[k, v] : options.params)
      {
        merged.params[k] = v;
      }

      if (!options.body.empty())
      {
        merged.body = options.body;
      }

      if (!options.json.empty())
      {
        merged.json = options.json;
      }

      if (!options.form.empty())
      {
        merged.form = options.form;
      }

      if (!options.username.empty() || !options.password.empty())
      {
        merged.username = options.username;
        merged.password = options.password;
      }

      if (options.timeout_seconds > 0)
      {
        merged.timeout_seconds = options.timeout_seconds;
      }

      merged.follow_redirects = options.follow_redirects;
      merged.verify_tls = options.verify_tls;

      if (!options.user_agent.empty())
      {
        merged.user_agent = options.user_agent;
      }

      merged.compressed = options.compressed;

      return merged;
    }
  };

} // namespace vix::requests

#endif // VIX_REQUESTS_REQUESTS_HPP

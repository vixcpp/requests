#include <vix/async/core/io_context.hpp>
#include <vix/requests/requests.hpp>

#include "testing/ClientTestAccess.hpp"

#include <openssl/pem.h>
#include <openssl/ssl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
  constexpr char certificate[] = R"(-----BEGIN CERTIFICATE-----
MIIDCTCCAfGgAwIBAgIUfxB0rQw69Ldycmh/2VoK8u9ELwIwDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJMTI3LjAuMC4xMB4XDTI2MDkwOTEyMTMwNVoXDTI2MDkx
MDEyMTMwNVowFDESMBAGA1UEAwwJMTI3LjAuMC4xMIIBIjANBgkqhkiG9w0BAQEF
AAOCAQ8AMIIBCgKCAQEAsUcLY2TAWkc/uPba72jmq5y695CVs7xNxOSYCCHH3YU1
41DZbBrk/tTy+fpfhEF1oJMegZdZKlsL97yiU+XluK1lM69yJ1RodCXys4A5YU0y
AnndQpTG5dV8mTY2jqtYM5aizr4UEKVstIgIq0wQdFbYAS97e34oUNkcuDkPk+cI
ZYCT6Amci1B68gf6EjMr8MHOC2asl/j9v7oVdzr0Jc4WMo6vHhVVgxz2Z9npJFe/
PQ5I7uzScD7kXHhmCF1SPAMslga/9l8sIsAsK8Yv2ShFrp/nGJKmGSa13dOB5AB7
Jkzts5tMNv9M+wSG4X/32Lv/tOPwT0vTVLiIrhUdKwIDAQABo1MwUTAdBgNVHQ4E
FgQUdcGSENASRQI+9ZUK+KyzKiVHdyEwHwYDVR0jBBgwFoAUdcGSENASRQI+9ZUK
+KyzKiVHdyEwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAiIms
hVRjx8sxPEnjLxmeHbyWsSG+MUwYyBMqgDch7zyvXKwKWAQLU5QpiWVA0UAcn+GP
Ig42IRHqwAj1r1H6EUuxf3B35B6QrON8vTAYz3p14pV+F0ZQ5OqCLHVjY6o2pj83
Yu0tOd3p0fh1egsK4ZRwysM/9dW9VJxk9Ibjpa5euHGxUrs5jqS5rriGk4xl/3px
KtBkhO00AK39wLTkWQqvUFNsXb4vurGB5DK3XPQafUFfYbXyBBTbq1cPdHSxfnrh
xxnt5wxoBYcCz7unJ84lyoJtzp+XbXYDGow+2VArWHDxMZHzOmJV5tVhVXkOgSKf
7/AbgSHfJvwb3ImYag==
-----END CERTIFICATE-----
)";
  constexpr char private_key[] = R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCxRwtjZMBaRz+4
9trvaOarnLr3kJWzvE3E5JgIIcfdhTXjUNlsGuT+1PL5+l+EQXWgkx6Bl1kqWwv3
vKJT5eW4rWUzr3InVGh0JfKzgDlhTTICed1ClMbl1XyZNjaOq1gzlqLOvhQQpWy0
iAirTBB0VtgBL3t7fihQ2Ry4OQ+T5whlgJPoCZyLUHryB/oSMyvwwc4LZqyX+P2/
uhV3OvQlzhYyjq8eFVWDHPZn2ekkV789Dkju7NJwPuRceGYIXVI8AyyWBr/2Xywi
wCwrxi/ZKEWun+cYkqYZJrXd04HkAHsmTO2zm0w2/0z7BIbhf/fYu/+04/BPS9NU
uIiuFR0rAgMBAAECggEAMBsPRBF7AQSEjjSuhHj632bQdY2Okg9vKxa9Crff/ITB
csXluigyK9Stepgugz0gJkrF5HSlhZDfVoKhqhKgm9SxuazKVVHaZmpFyd/psjW4
dGONXxvwPT4JaaoEdAKbFKSz2VZLK5k4PmULtTAW5vsJ8ohoAcLiPc4stPEHASMk
a0UvF1lLgh23tw3orScJIVDXEi9iu/sbGuaLC5QPsIHwyix90TzRIszCmyPU9t2m
CBq6I2/z13XyrQgq9dOESwiSKDdJSynPvKfjw7XgxSo4QQ6DAAuMycjbzwXqVy7/
DMW1Xt2MIbZQD1jaLEjvoJsPSqCxn18DIxlwwDt1wQKBgQDnJanXAAcrwFiT7uiX
YYCBmMmqOaEoLlrRe90oUGSGxfS2PTEeDaRARKhYMLgDt7uuqY8eFWVe0coW+eIu
7BSdQTNMvUzwV0800ZAZ6q9jnwbck7NahaNY3p+TGzvFI0DRX4qWOBZFcQFdeuHG
eU4q3Jse0TMhMUI0rC5XnNyUdwKBgQDEVp4LnDqY7znczzhDjZQOwgDFdcf63Qn5
vRo4jP9DC4dcyjhBSo84fQR9ZXL3lKXIZ0O2cx8+FAIwCLil7EjKpo9s5+f35FgD
qUpSWKxTYXjamv/WaX+G9kWlAefea9tTsqciGQQen0NZ8RgikHSm9cGHGR/9/IRY
CPpNQd9t7QKBgQDXACp1SJpi8LuzhlKklFWytgpGouoq9MsW9W6l0/ldfv37EPoB
6VBxUyt/NHnFQ354nVD4+4/Z3atrKa1gVclq6D2uUJojKCMNjyvJX4kaUc5RF7WJ
Kqrgu9qUO42h9gORfJvs1Giq2M0bRXSCZ0z1A1PqG07Cvg3F7q4AYnCuSwKBgQCF
DFlfbsg6uOpSGs564KczdP5G0TDqa/2FkkMBjXKxgU+Cbrq+bAMv+XAgda353m6V
pny5V7wVOkPdPxu5vKv7P/GzkFeeocuO95qYwOROQKBAK4cxSh6UW7EIR4lULWzc
hBVEQwWKpsxiV8HOMmwKdEFoBnuqhwM8ESFlVz1MZQKBgB5ih1ehM/qnhuzmYKFX
9T56Fysc4EAljnHwqXKdSfkXxBTaTSmHnMTbUFMKe/dKa03JKxjuXGsYi8T/Y5yq
pMfHgchiYnIopfgkEATiGMe2LPnI/bghgKbp5ZtNOyiwgUHomU/7sn/lPzetm8gW
p9n/UmE4H6T5cuCjYJiUmDzl
-----END PRIVATE KEY-----
)";

  void expect(bool condition, const char *message)
  {
    if (!condition) throw std::runtime_error(message);
  }

  class LocalHttpsServer
  {
  public:
    enum class Mode { KeepAlive, CloseAfterResponse, ConnectionClose };

    explicit LocalHttpsServer(Mode mode) : mode_(mode)
    {
      SSL_CTX *context = SSL_CTX_new(TLS_server_method());
      if (!context) throw std::runtime_error("SSL_CTX_new failed");
      ctx_ = context;
      BIO *cert_bio = BIO_new_mem_buf(certificate, -1);
      BIO *key_bio = BIO_new_mem_buf(private_key, -1);
      X509 *cert = PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
      EVP_PKEY *key = PEM_read_bio_PrivateKey(key_bio, nullptr, nullptr, nullptr);
      BIO_free(cert_bio); BIO_free(key_bio);
      if (!cert || !key || SSL_CTX_use_certificate(ctx_, cert) != 1 ||
          SSL_CTX_use_PrivateKey(ctx_, key) != 1)
        throw std::runtime_error("test TLS credential setup failed");
      X509_free(cert); EVP_PKEY_free(key);
      fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      int enabled = 1;
      ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
      sockaddr_in address{}; address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      if (::bind(fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
          ::listen(fd_, 8) != 0) throw std::runtime_error(std::strerror(errno));
      socklen_t length = sizeof(address);
      ::getsockname(fd_, reinterpret_cast<sockaddr *>(&address), &length);
      port_ = ntohs(address.sin_port);
      thread_ = std::thread([this] { serve(); });
    }
    ~LocalHttpsServer()
    {
      running_ = false; ::shutdown(fd_, SHUT_RDWR); ::close(fd_);
      if (thread_.joinable()) thread_.join();
      SSL_CTX_free(ctx_);
    }
    std::string url() const { return "https://127.0.0.1:" + std::to_string(port_) + "/"; }
    int connections() const { return connections_; }
  private:
    void serve()
    {
      while (running_)
      {
        const int client = ::accept(fd_, nullptr, nullptr);
        if (client < 0) continue;
        ++connections_;
        SSL *ssl = SSL_new(ctx_); SSL_set_fd(ssl, client);
        if (SSL_accept(ssl) == 1)
          while (running_)
          {
            std::string request; char buffer[1024];
            while (request.find("\r\n\r\n") == std::string::npos)
            {
              const int count = SSL_read(ssl, buffer, sizeof(buffer));
              if (count <= 0) break;
              request.append(buffer, static_cast<std::size_t>(count));
            }
            if (request.empty()) break;
            const char *connection = mode_ == Mode::ConnectionClose ? "Connection: close\r\n" : "";
            const std::string response = std::string("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n") +
                connection + "\r\nok";
            SSL_write(ssl, response.data(), static_cast<int>(response.size()));
            if (mode_ != Mode::KeepAlive) break;
          }
        SSL_shutdown(ssl); SSL_free(ssl); ::close(client);
      }
    }
    Mode mode_; SSL_CTX *ctx_{}; int fd_{}; std::uint16_t port_{}; std::atomic<bool> running_{true};
    std::atomic<int> connections_{0}; std::thread thread_;
  };

  vix::async::core::task<void> get_many(vix::async::core::io_context &ctx,
      vix::requests::Client &client, const std::string &url, int count,
      vix::requests::RequestOptions options, std::exception_ptr &error)
  {
    try { for (int i = 0; i < count; ++i) expect((co_await client.async_get(ctx, url, options)).ok(), "HTTPS GET failed"); }
    catch (...) { error = std::current_exception(); }
    ctx.stop(); co_return;
  }
  void run_gets(vix::requests::Client &client, const std::string &url, int count)
  {
    vix::async::core::io_context ctx; std::exception_ptr error; vix::requests::RequestOptions options; options.verify_tls = false;
    auto task = get_many(ctx, client, url, count, options, error); ctx.post(task.handle()); ctx.run();
    if (error) std::rethrow_exception(error);
  }
  void test_https_keep_alive()
  {
    LocalHttpsServer server(LocalHttpsServer::Mode::KeepAlive); vix::requests::Client client;
    run_gets(client, server.url(), 10);
    const auto counters = vix::requests::testing::ClientTestAccess::connection_counters(client);
    expect(counters.connections_created == 1 && counters.connections_reused == 9, "HTTPS keep-alive counters must be 1/9");
    expect(server.connections() == 1, "HTTPS keep-alive must perform one handshake");
  }
  void test_stale_https_get()
  {
    LocalHttpsServer server(LocalHttpsServer::Mode::CloseAfterResponse); vix::requests::Client client;
    run_gets(client, server.url(), 2);
    const auto counters = vix::requests::testing::ClientTestAccess::connection_counters(client);
    expect(counters.connections_created == 2 && counters.connections_reused == 1, "stale HTTPS GET must retry once on a fresh connection");
    expect(server.connections() == 2, "stale HTTPS GET must establish a replacement connection");
  }
  void test_https_connection_close()
  {
    LocalHttpsServer server(LocalHttpsServer::Mode::ConnectionClose); vix::requests::Client client;
    run_gets(client, server.url(), 2);
    const auto counters = vix::requests::testing::ClientTestAccess::connection_counters(client);
    expect(counters.connections_created == 2 && counters.connections_reused == 0, "Connection: close must not enter the idle pool");
  }
}
int main()
{
  try { test_https_keep_alive(); test_stale_https_get(); test_https_connection_close(); std::cout << "connection_lifecycle_test passed\n"; return 0; }
  catch (const std::exception &error) { std::cerr << "connection_lifecycle_test failed: " << error.what() << '\n'; return 1; }
}

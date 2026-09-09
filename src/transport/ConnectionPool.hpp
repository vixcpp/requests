/** Internal, non-blocking HTTP/1.1 idle-connection store. */
#ifndef VIX_REQUESTS_TRANSPORT_CONNECTION_POOL_HPP
#define VIX_REQUESTS_TRANSPORT_CONNECTION_POOL_HPP

#include <vix/requests/Url.hpp>
#include <vix/requests/transport/Transport.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace vix::requests::transport
{
  struct ConnectionCounters
  {
    std::size_t connections_created{};
    std::size_t connections_reused{};
  };

  struct AcquiredConnection
  {
    TransportPtr transport;
    bool reused{};
  };

  class ConnectionPool
  {
  public:
    [[nodiscard]] AcquiredConnection acquire(const Url &url);
    [[nodiscard]] AcquiredConnection acquire_async(
        const Url &url, const vix::async::core::io_context &ctx);
    void release(const Url &url, TransportPtr transport);
    void discard(TransportPtr transport) noexcept;
    [[nodiscard]] ConnectionCounters counters() const noexcept;

  private:
    [[nodiscard]] static std::string key(const Url &url);
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<TransportPtr>> idle_;
    std::atomic_size_t created_{0};
    std::atomic_size_t reused_{0};
  };
}
#endif

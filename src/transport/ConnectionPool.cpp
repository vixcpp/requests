#include "transport/ConnectionPool.hpp"
#include "transport/TransportFactory.hpp"

namespace vix::requests::transport
{
  std::string ConnectionPool::key(const Url &url)
  {
    return url.scheme() + "://" + url.host() + ":" + std::to_string(url.port());
  }

  AcquiredConnection ConnectionPool::acquire(const Url &url)
  {
    const std::string origin = key(url);
    {
      std::lock_guard lock(mutex_);
      auto found = idle_.find(origin);
      if (found != idle_.end() && !found->second.empty())
      {
        TransportPtr transport = std::move(found->second.back());
        found->second.pop_back();
        reused_.fetch_add(1, std::memory_order_relaxed);
        return {std::move(transport), true};
      }
    }
    created_.fetch_add(1, std::memory_order_relaxed);
    return {make_transport_for_url(url), false};
  }

  AcquiredConnection ConnectionPool::acquire_async(
      const Url &url, const vix::async::core::io_context &ctx)
  {
    const std::string origin = key(url);
    {
      std::lock_guard lock(mutex_);
      auto found = idle_.find(origin);
      if (found != idle_.end())
      {
        for (auto it = found->second.begin(); it != found->second.end(); ++it)
        {
          if ((*it)->async_compatible(ctx))
          {
            TransportPtr transport = std::move(*it);
            found->second.erase(it);
            reused_.fetch_add(1, std::memory_order_relaxed);
            return {std::move(transport), true};
          }
        }
      }
    }
    created_.fetch_add(1, std::memory_order_relaxed);
    return {make_transport_for_url(url), false};
  }

  void ConnectionPool::release(const Url &url, TransportPtr transport)
  {
    if (!transport || !transport->reusable())
    {
      discard(std::move(transport));
      return;
    }
    std::lock_guard lock(mutex_);
    idle_[key(url)].push_back(std::move(transport));
  }

  void ConnectionPool::discard(TransportPtr transport) noexcept
  {
    if (transport) transport->discard();
  }

  ConnectionCounters ConnectionPool::counters() const noexcept
  {
    return {created_.load(std::memory_order_relaxed), reused_.load(std::memory_order_relaxed)};
  }
}

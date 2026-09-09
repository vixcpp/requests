/** Test-only access to client transport instrumentation. */
#ifndef VIX_REQUESTS_TESTING_CLIENT_TEST_ACCESS_HPP
#define VIX_REQUESTS_TESTING_CLIENT_TEST_ACCESS_HPP

#include <vix/requests/Client.hpp>

#include "transport/ConnectionPool.hpp"

namespace vix::requests::testing
{
  struct ClientTestAccess
  {
    [[nodiscard]] static transport::ConnectionCounters connection_counters(
        const Client &client) noexcept
    {
      return client.pool_->counters();
    }
  };
}

#endif // VIX_REQUESTS_TESTING_CLIENT_TEST_ACCESS_HPP

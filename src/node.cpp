// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018-2019 IncludeOS AS, Oslo, Norway
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "node.hpp"
#include "balancer.hpp"

// checking if nodes are dead or not
#define ACTIVE_INITIAL_PERIOD     8s
#define ACTIVE_CHECK_PERIOD      30s
// connection attempt timeouts
#define CONNECT_TIMEOUT          10s

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf("MICROLB: "); printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

namespace microLB
{
  Node::Node(Balancer& balancer, const net::Socket addr,
             node_connect_function_t func, bool da, int idx)
    : m_connect(func), m_socket(addr), m_idx(idx), do_active_check(da)
  {
    assert(this->m_connect != nullptr);
    this->m_pool_signal = balancer.get_pool_signal();
    // periodically connect to node and determine if active
    if (this->do_active_check)
    {
      // however, perform first check immediately
      this->active_timer = Timers::periodic(0s, ACTIVE_CHECK_PERIOD,
                           {this, &Node::perform_active_check});
    }
  }
  void Node::perform_active_check(int)
  {
    assert(this->do_active_check);
    try {
      this->connect();
    } catch (std::exception& e) {
      // do nothing, because might be eph.ports used up
      LBOUT("Node %d exception %s\n", this->m_idx, e.what());
    }
  }
  void Node::restart_active_check()
  {
    // set as inactive
    this->active = false;
    if (this->do_active_check)
    {
      // begin checking active again
      if (this->active_timer == Timers::UNUSED_ID)
      {
        this->active_timer = Timers::periodic(
          ACTIVE_INITIAL_PERIOD, ACTIVE_CHECK_PERIOD,
          {this, &Node::perform_active_check});

        LBOUT("Node %s restarting active check (and is inactive)\n",
              this->addr.to_string().c_str());
      }
      else
      {
        LBOUT("Node %s still trying to connect...\n",
              this->addr.to_string().c_str());
      }
    }
  }
  void Node::stop_active_check()
  {
    // set as active
    this->active = true;
    if (this->do_active_check)
    {
      // stop active checking for now
      if (this->active_timer != Timers::UNUSED_ID) {
        Timers::stop(this->active_timer);
        this->active_timer = Timers::UNUSED_ID;
      }
    }
    LBOUT("Node %d stopping active check (and is active)\n", this->m_idx);
  }
  void Node::connect()
  {
    // connecting to node atm.
    this->connecting++;
    this->m_connect(CONNECT_TIMEOUT,
      [this] (net::Stream_ptr stream)
      {
        // no longer connecting
        assert(this->connecting > 0);
        this->connecting --;
        // success
        if (stream != nullptr)
        {
          assert(stream->is_connected());
          LBOUT("Node %d connected to %s (%ld total)\n",
                this->m_idx, stream->remote().to_string().c_str(), pool.size());
          this->pool.push_back(std::move(stream));
          // stop any active check
          this->stop_active_check();
          // signal change in pool
          this->m_pool_signal();
        }
        else // failure
        {
          LBOUT("Node %d failed to connect out (%ld total)\n",
                this->m_idx, pool.size());
          // restart active check
          this->restart_active_check();
        }
      });
  }
  net::Stream_ptr Node::get_connection()
  {
    while (pool.empty() == false) {
      auto conn = std::move(pool.back());
      assert(conn != nullptr);
      pool.pop_back();
      if (conn->is_connected()) {
        return conn;
      }
      else
      {
        printf("CLOSING SINCE conn->connected is false\n");
        conn->close();
      }
    }
    return nullptr;
  }
}


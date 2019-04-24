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

#pragma once
#include <net/stream.hpp>
#include <vector>
#include <chrono>

namespace microLB
{
  typedef std::chrono::milliseconds timeout_t;
  typedef delegate<void(net::Stream_ptr)> node_connect_result_t;
  typedef delegate<void(timeout_t, node_connect_result_t)> node_connect_function_t;
  typedef delegate<void()> pool_signal_t;

  struct Balancer;
  struct Node {
    Node(Balancer&, net::Socket, node_connect_function_t,
         bool do_active = true, int idx = -1);

    auto address() const noexcept { return m_socket; }
    int  connection_attempts() const noexcept { return this->connecting; }
    int  pool_size() const noexcept { return pool.size(); }
    bool is_active() const noexcept { return active; }
    bool active_check() const noexcept { return do_active_check; }

    void restart_active_check();
    void perform_active_check(int);
    void stop_active_check();
    void connect();
    net::Stream_ptr get_connection();

  private:
    node_connect_function_t m_connect = nullptr;
    pool_signal_t           m_pool_signal = nullptr;
    std::vector<net::Stream_ptr> pool;
    net::Socket m_socket;
    int         m_idx;
    bool        active = false;
    const bool  do_active_check;
    int32_t     active_timer = -1;
    int32_t     connecting = 0;
  };
}


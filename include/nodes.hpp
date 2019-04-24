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
#include "node.hpp"
#include "session.hpp"
#include <util/timer.hpp>
#include <net/inet>
#include <deque>

namespace liu {
  struct Storage;
  struct Restore;
  using buffer_t = std::vector<char>;
}

namespace microLB
{
  struct DeserializationHelper
  {
    net::Inet* clients = nullptr;
    net::Inet* nodes   = nullptr;
    void* cli_ctx = nullptr;
    void* nod_ctx = nullptr;
  };

  struct Balancer;
  struct Nodes {
    typedef std::deque<Node> nodevec_t;
    typedef nodevec_t::iterator iterator;
    typedef nodevec_t::const_iterator const_iterator;

    Nodes(Balancer& b, bool ac) : m_lb(b), do_active_check(ac) {}

    inline size_t   size() const noexcept;
    inline const_iterator begin() const;
    inline const_iterator end() const;
    inline int32_t open_sessions() const noexcept;
    inline int64_t total_sessions() const noexcept;
    inline int32_t timed_out_sessions() const noexcept;
    int  pool_connecting() const;
    int  pool_size() const;

    template <typename... Args>
    void add_node(Args&&... args);
    void create_connections(int total);
    // returns the connection back if the operation fails
    net::Stream_ptr assign(net::Stream_ptr);
    Session& create_session(net::Stream_ptr inc, net::Stream_ptr out);
    void     close_session(int);
    void destroy_sessions();
    Session& get_session(int);
    void     close_all_sessions();
#if defined(LIVEUPDATE)
    void serialize(liu::Storage&);
    void deserialize(liu::Restore&, DeserializationHelper&);
#endif
    // make the microLB more testable
    delegate<void(int idx, int current, int total)> on_session_close = nullptr;

  private:
    Balancer& m_lb;
    nodevec_t nodes;
    int64_t   session_total = 0;
    int       session_cnt = 0;
    int       conn_iterator = 0;
    int       algo_iterator = 0;
    const bool do_active_check;
    Timer cleanup_timer;
    std::deque<Session> sessions;
    std::deque<int> free_sessions;
    std::deque<int> closed_sessions;
  };

  template <typename... Args>
  inline void Nodes::add_node(Args&&... args) {
    nodes.emplace_back(m_lb, std::forward<Args> (args)...,
                       this->do_active_check, nodes.size());
  }

  size_t Nodes::size() const noexcept
  { return nodes.size(); }
  Nodes::const_iterator Nodes::begin() const
  { return nodes.cbegin(); }
  Nodes::const_iterator Nodes::end() const
  { return nodes.cend(); }
  int32_t Nodes::open_sessions() const noexcept
  { return session_cnt; }
  int64_t Nodes::total_sessions() const noexcept
  { return session_total; }
  int32_t Nodes::timed_out_sessions() const noexcept
  { return 0; }
}

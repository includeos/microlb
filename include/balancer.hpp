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

#include "nodes.hpp"
namespace net {
  class Inet;
}

namespace microLB
{
  typedef net::Inet netstack_t;

  struct Waiting {
    Waiting(net::Stream_ptr);
#if defined(LIVEUPDATE)
    Waiting(liu::Restore&, DeserializationHelper&);
    void serialize(liu::Storage&);
#endif

    net::Stream_ptr conn;
    int total = 0;
  };

  struct Balancer {
    Balancer(bool active_check);
    ~Balancer();

    static Balancer* from_config();

    // Frontend/Client-side of the load balancer
    void open_for_tcp(netstack_t& interface, uint16_t port);
    void open_for_s2n(netstack_t& interface, uint16_t port, const std::string& cert, const std::string& key);
    void open_for_ossl(netstack_t& interface, uint16_t port, const std::string& cert, const std::string& key);
    // Backend/Application side of the load balancer
    static node_connect_function_t connect_with_tcp(netstack_t& interface, net::Socket);
    // Setup and automatic resume (if applicable)
    // NOTE: Be sure to have configured it properly BEFORE calling this

    inline int  wait_queue() const;
    inline int  connect_throws() const noexcept;
    // add a client stream to the load balancer
    // NOTE: the stream must be connected prior to calling this function
    void incoming(net::Stream_ptr);

#if defined(LIVEUPDATE)
    void init_liveupdate();
    void serialize(liu::Storage&, const liu::buffer_t*);
    void resume_callback(liu::Restore&);
#endif

    Nodes nodes;
    inline pool_signal_t get_pool_signal();
    DeserializationHelper de_helper;

  private:
    void handle_connections();
    void handle_queue();
#if defined(LIVEUPDATE)
     void deserialize(liu::Restore&);
#endif
    std::vector<net::Socket> parse_node_confg();

    std::deque<Waiting> queue;
    int throw_retry_timer = -1;
    int throw_counter = 0;
    // TLS stuff (when enabled)
    void* tls_context = nullptr;
    delegate<void()> tls_free = nullptr;
  };

  int Balancer::wait_queue() const
  { return this->queue.size(); }
  int Balancer::connect_throws() const noexcept
  { return this->throw_counter; }
  pool_signal_t Balancer::get_pool_signal()
  { return {this, &Balancer::handle_queue}; }

}

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

#include "balancer.hpp"
#include <os.hpp>

#define MAX_OUTGOING_ATTEMPTS    100
#define CONNECT_THROW_PERIOD     20s

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf("MICROLB: "); printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

using namespace std::chrono;

// NOTE: Do NOT move microLB::Balancer while in operation!
// It uses tons of delegates that capture "this"
namespace microLB
{
  Balancer::Balancer(const bool da) : nodes {*this, da}  {}
  Balancer::~Balancer()
  {
    queue.clear();
    nodes.close_all_sessions();
    if (tls_free) tls_free();
  }
  void Balancer::incoming(net::Stream_ptr conn)
  {
      assert(conn != nullptr);
      queue.emplace_back(std::move(conn));
      LBOUT("Queueing connection (q=%lu)\n", queue.size());
      // IMPORTANT: try to handle queue, in case its ready
      // don't directly call handle_connections() from here!
      this->handle_queue();
  }
  void Balancer::handle_queue()
  {
    // check waitq
    while (nodes.pool_size() > 0 && queue.empty() == false)
    {
      auto& client = queue.front();
      assert(client.conn != nullptr);
      if (client.conn->is_connected()) {
        try {
          // NOTE: explicitly want to copy buffers
          net::Stream_ptr rval =
              nodes.assign(std::move(client.conn));
          if (rval == nullptr) {
            // done with this queue item
            queue.pop_front();
          }
          else {
            // put connection back in queue item
            client.conn = std::move(rval);
          }
        } catch (...) {
          queue.pop_front(); // we have no choice
          throw;
        }
      }
      else {
        queue.pop_front();
      }
    } // waitq check
    // check if we need to create more connections
    this->handle_connections();
  }
  void Balancer::handle_connections()
  {
    LBOUT("Handle_connections. %lu waiting \n", queue.size());
    // stop any rethrow timer since this is a de-facto retry
    if (this->throw_retry_timer != Timers::UNUSED_ID) {
        Timers::stop(this->throw_retry_timer);
        this->throw_retry_timer = Timers::UNUSED_ID;
    }

    // prune dead clients because the "number of clients" is being
    // used in a calculation right after this to determine how many
    // nodes to connect to
    auto new_end = std::remove_if(queue.begin(), queue.end(),
        [](Waiting& client) {
          return client.conn == nullptr || client.conn->is_connected() == false;
        });
    queue.erase(new_end, queue.end());

    // calculating number of connection attempts to create
    int np_connecting = nodes.pool_connecting();
    int estimate = queue.size() - (np_connecting + nodes.pool_size());
    estimate = std::min(estimate, MAX_OUTGOING_ATTEMPTS);
    estimate = std::max(0, estimate - np_connecting);
    // create more outgoing connections
    LBOUT("Estimated connections needed: %d\n", estimate);
    if (estimate > 0)
    {
      try {
        nodes.create_connections(estimate);
      }
      catch (std::exception& e)
      {
        this->throw_counter++;
        // assuming the failure is due to not enough eph. ports
        this->throw_retry_timer = Timers::oneshot(CONNECT_THROW_PERIOD,
        [this] (int) {
            this->throw_retry_timer = Timers::UNUSED_ID;
            this->handle_connections();
        });
      }
    } // estimate
  } // handle_connections()

#if !defined(LIVEUPDATE)
  //if we dont support liveupdate then do nothing
  void init_liveupdate() {}
#endif

  Waiting::Waiting(net::Stream_ptr incoming)
    : conn(std::move(incoming)), total(0)
  {
    assert(this->conn != nullptr);
    assert(this->conn->is_connected());

    // Release connection if it closes before it's assigned to a node.
    this->conn->on_close([this](){
        LBOUT("Waiting issuing close\n");
        if (this->conn != nullptr)
          this->conn->reset_callbacks();
        this->conn = nullptr;
      });
  }
}

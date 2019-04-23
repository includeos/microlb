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

#include "nodes.hpp"
#include <net/tcp/stream.hpp>

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf("MICROLB: "); printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

namespace microLB
{
  void Nodes::create_connections(int total)
  {
    // temporary iterator
    for (int i = 0; i < total; i++)
    {
      bool dest_found = false;
      // look for next active node up to *size* times
      for (size_t i = 0; i < nodes.size(); i++)
      {
        const int iter = conn_iterator;
        conn_iterator = (conn_iterator + 1) % nodes.size();
        // if the node is active, connect immediately
        auto& dest_node = nodes[iter];
        if (dest_node.is_active()) {
          dest_node.connect();
          dest_found = true;
          break;
        }
      }
      // if no active node found, simply delegate to the next node
      if (dest_found == false)
      {
        // with active-checks we can return here later when we get a connection
        if (this->do_active_check) return;
        const int iter = conn_iterator;
        conn_iterator = (conn_iterator + 1) % nodes.size();
        nodes[iter].connect();
      }
    }
  }
  net::Stream_ptr Nodes::assign(net::Stream_ptr conn)
  {
    for (size_t i = 0; i < nodes.size(); i++)
    {
      auto outgoing = nodes[algo_iterator].get_connection();
      // algorithm here //
      algo_iterator = (algo_iterator + 1) % nodes.size();
      // check if connection was retrieved
      if (outgoing != nullptr)
      {
        assert(outgoing->is_connected());
        LBOUT("Assigning client to node %d (%s)\n",
              algo_iterator, outgoing->to_string().c_str());
        //Should we some way hold track of the session object ?
        auto& session = this->create_session(
              std::move(conn), std::move(outgoing));

        return nullptr;
      }
    }
    return conn;
  }
  size_t Nodes::size() const noexcept {
    return nodes.size();
  }
  Nodes::const_iterator Nodes::begin() const {
    return nodes.cbegin();
  }
  Nodes::const_iterator Nodes::end() const {
    return nodes.cend();
  }
  int Nodes::pool_connecting() const {
    int count = 0;
    for (auto& node : nodes) count += node.connection_attempts();
    return count;
  }
  int Nodes::pool_size() const {
    int count = 0;
    for (auto& node : nodes) count += node.pool_size();
    return count;
  }
  int32_t Nodes::open_sessions() const {
    return session_cnt;
  }
  int64_t Nodes::total_sessions() const {
    return session_total;
  }
  int32_t Nodes::timed_out_sessions() const {
    return 0;
  }
  Session& Nodes::create_session(net::Stream_ptr client, net::Stream_ptr outgoing)
  {
    int idx = -1;
    if (free_sessions.empty()) {
      idx = sessions.size();
      sessions.emplace_back(*this, idx, std::move(client), std::move(outgoing));
    } else {
      idx = free_sessions.back();
      new (&sessions[idx]) Session(*this, idx, std::move(client), std::move(outgoing));
      free_sessions.pop_back();
    }
    session_total++;
    session_cnt++;
    LBOUT("New session %d  (current = %d, total = %ld)\n",
          idx, session_cnt, session_total);
    return sessions[idx];
  }
  Session& Nodes::get_session(int idx)
  {
    auto& session = sessions.at(idx);
    assert(session.is_alive());
    return session;
  }

  void Nodes::destroy_sessions()
  {
    for (auto& idx: closed_sessions)
    {
      auto &session=get_session(idx);

      // free session destroying potential unique ptr objects
      session.incoming = nullptr;
      auto out_tcp = dynamic_cast<net::tcp::Stream*>(session.outgoing->bottom_transport())->tcp();
      session.outgoing = nullptr;
      // if we don't have anything to write to the backend, abort it.
      if(not out_tcp->sendq_size())
        out_tcp->abort();
      free_sessions.push_back(session.self);
      LBOUT("Session %d destroyed  (total = %d)\n", session.self, session_cnt);
    }
    closed_sessions.clear();
  }
  void Nodes::close_session(int idx)
  {
    auto& session = get_session(idx);
    // remove connections
    session.incoming->reset_callbacks();
    session.outgoing->reset_callbacks();
    closed_sessions.push_back(session.self);

    destroy_sessions();

    session_cnt--;
    LBOUT("Session %d closed  (total = %d)\n", session.self, session_cnt);
    if (on_session_close) on_session_close(session.self, session_cnt, session_total);
  }
  void Nodes::close_all_sessions()
  {
    sessions.clear();
    free_sessions.clear();
  }
}

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

#include "microLB"
#include <net/inet>
#include <net/tcp/stream.hpp>

namespace microLB
{
  // default method for opening a TCP port for clients
  void Balancer::open_for_tcp(
        netstack_t&    interface,
        const uint16_t client_port)
  {
    interface.tcp().listen(client_port,
    [this] (net::tcp::Connection_ptr conn) {
      assert(conn != nullptr && "TCP sanity check");
      this->incoming(std::make_unique<net::tcp::Stream> (conn));
    });

    this->de_helper.clients = &interface;
    //this->de_helper.cli_ctx = nullptr;
  }
  // default method for TCP nodes
  node_connect_function_t Balancer::connect_with_tcp(
          netstack_t& interface,
          net::Socket socket)
  {
return [&interface, socket] (timeout_t timeout, node_connect_result_t callback)
    {
      net::tcp::Connection_ptr conn;
      try
      {
        conn = interface.tcp().connect(socket);
      }
      catch([[maybe_unused]]const net::TCP_error& err)
      {
        //printf("Got exception: %s\n", err.what());
        callback(nullptr);
        return;
      }
      assert(conn != nullptr && "TCP sanity check");
      // cancel connect after timeout
      int timer = Timers::oneshot(timeout,
          Timers::handler_t::make_packed(
          [conn, callback] (int) {
            conn->abort();
            callback(nullptr);
          }));
      conn->on_connect(
        net::tcp::Connection::ConnectCallback::make_packed(
        [timer, callback] (net::tcp::Connection_ptr conn) {
          // stop timeout after successful connect
          Timers::stop(timer);
          if (conn != nullptr) {
            // the connect() succeeded
            assert(conn->is_connected() && "TCP sanity check");
            callback(std::make_unique<net::tcp::Stream>(conn));
          }
          else {
            // the connect() failed
            callback(nullptr);
          }
        }));
    };
  }
}

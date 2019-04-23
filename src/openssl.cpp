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
#include <memdisk>
#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include <net/inet>
#include <net/tcp/stream.hpp>

namespace microLB
{
  void Balancer::open_for_ossl(
        netstack_t&        interface,
        const uint16_t     client_port,
        const std::string& tls_cert,
        const std::string& tls_key)
  {
    fs::memdisk().init_fs(
    [] (fs::error_t err, fs::File_system&) {
      assert(!err);
    });

    openssl::init();
    openssl::verify_rng();

    this->tls_context = openssl::create_server(tls_cert, tls_key);

    interface.tcp().listen(client_port,
      [this] (net::tcp::Connection_ptr conn) {
        if (conn != nullptr)
        {
          auto* stream = new openssl::TLS_stream(
              (SSL_CTX*) this->tls_context,
              std::make_unique<net::tcp::Stream>(conn)
          );
          stream->on_connect(
            [this, stream] (auto&) {
              this->incoming(std::unique_ptr<openssl::TLS_stream> (stream));
            });
          stream->on_close(
            [stream] () {
              delete stream;
            });
        }
      });
  } // open_ossl(...)
}

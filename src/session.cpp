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

#include "session.hpp"
#include "nodes.hpp"

namespace microLB
{
  // use indexing to access Session because std::vector
  Session::Session(Nodes& n, int idx,
                   net::Stream_ptr inc, net::Stream_ptr out)
      : parent(n), self(idx), incoming(std::move(inc)),
                              outgoing(std::move(out))
  {
    incoming->on_data({this, &Session::flush_incoming});
    incoming->on_close(
    [&nodes = n, idx] () {
        nodes.close_session(idx);
    });

    outgoing->on_data({this, &Session::flush_outgoing});
    outgoing->on_close(
    [&nodes = n, idx] () {
        nodes.close_session(idx);
    });
  }
  bool Session::is_alive() const {
    return incoming != nullptr;
  }

  void Session::flush_incoming()
  {
    assert(this->is_alive());
    while((this->incoming->next_size() > 0) and this->outgoing->is_writable())
    {
      this->outgoing->write(this->incoming->read_next());
    }
  }

  void Session::flush_outgoing()
  {
    assert(this->is_alive());
    while((this->outgoing->next_size() > 0) and this->incoming->is_writable())
    {
      this->incoming->write(this->outgoing->read_next());
    }
  }

}

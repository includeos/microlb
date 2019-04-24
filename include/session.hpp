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

namespace liu {
  struct Storage;
}
namespace microLB
{
  struct Nodes;
  struct Session {
    Session(Nodes&, int idx, net::Stream_ptr in, net::Stream_ptr out);
    inline bool is_alive() const noexcept;
#if defined(LIVEUPDATE)
    void serialize(liu::Storage&);
#endif

    Nodes&     parent;
    const int  self;
    net::Stream_ptr incoming;
    net::Stream_ptr outgoing;

    void flush_incoming();
    void flush_outgoing();
  };

  bool Session::is_alive() const noexcept
  { return incoming != nullptr; }
}

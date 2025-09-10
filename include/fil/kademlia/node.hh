// MIT License
//
// Copyright (c) 2025 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FiL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef FIL_NODE_HH
#define FIL_NODE_HH

#include <chrono>
#include <functional>

namespace fil::p2p::kademlia {

namespace details {
class bucket;

} // namespace details

class endpoint;

struct metadata_node {
    std::string user_agent;                                 //!< user agent of the node
    std::string description;                                //!< description provied by the node
    std::uint16_t version;                                  //!< version of the node
    std::chrono::steady_clock::time_point registering_time; //!< timestamp of the registering of the node
};

using node_id = std::array<std::uint8_t, 20>;               //!< node identifier (160 bits)
struct key {
    std::uint8_t type;                                      //!< type of the key (0 for sha1, 1 for sha256, etc.)
};

using store_cb             = std::function<void(const key&, const Value&)>;

using find_value_result_cb = std::function<void(std::optional<Value>)>;
using find_node_result_cb  = std::function<void(const std::vector<RemoteNode>&)>;

class node {
    friend struct api;

  public:
    explicit node(const std::vector<node_connection_info>& bootstrap);

    std::optional<node_id> find_node(const node_id& target);

  private:
    node_id id_;                                 //!< node identifier
    std::vector<details::bucket> nodes_buckets_; //!< list of endpoints to reach the node
};

} // namespace fil::p2p::kademlia

#endif // FIL_NODE_HH

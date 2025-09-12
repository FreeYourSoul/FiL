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

#include <bitset>
#include <chrono>
#include <future>
#include <ranges>
#include <unordered_map>

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/NObserver.h>
#include <Poco/Random.h>
#include <Poco/ThreadPool.h>
#include <Poco/Util/Timer.h>

#include "fil/datastructure/soa.hh"

// kademlia implementation following the specification described in the specification paper :
// https://cs.nyu.edu/~anirudh/CSCI-GA.2620-001/papers/kademlia.pdf

namespace fil::p2p::kademlia {
class endpoint {
  public:
    explicit endpoint(const std::string& ip, std::uint16_t port)
        : address(ip, port) {}

  private:
    Poco::Net::SocketAddress address;                   ///< POCO address (IP + port)
};

using node_id    = std::bitset<160>;                    //!< node identifier following the Kademlia specification (160 bits)
using data_key   = std::vector<std::uint8_t>;           //!< key used in the DHT
using data_value = std::vector<std::uint8_t>;           //!< value used in the DHT

struct configuration {
    std::size_t bucket_size                = 20;        //!< k‑bucket capacity
    std::size_t alpha                      = 3;         //!< parallel lookups per round
    std::uint16_t listen_port              = 4242;      //!< UDP port for inbound traffic
    std::string bind_interface             = "0.0.0.0"; //!< Interface to bind the socket
    std::chrono::milliseconds refresh_time = std::chrono::seconds {1800}; //!< time interval to refresh each bucket (30 min)

    std::vector<endpoint> initial_bootstrap;                              //!< list of endpoint to bootstrap the node
};

namespace details {
struct metadata_node {
    std::string user_agent;                                 //!< user agent of the node
    std::string description;                                //!< description provied by the node
    std::uint16_t version;                                  //!< version of the node
    std::chrono::steady_clock::time_point registering_time; //!< timestamp of the registering of the node
};

template<typename T>
concept value_policy =                 //
    std::is_default_constructible_v<T> //
    && requires(const T policy, const data_key& key) {
           { policy.get_value(key) } -> std::same_as<std::vector<std::uint8_t>>;
       };

/**
 * @param id  node id of the current node
 * @param other  node id of the other node
 * @return index of the bucket where the other node should be stored
 */
[[nodiscard]] inline std::size_t make_bucket_idx(const node_id& id, const node_id& other) {
    const auto leading_zeros = [&] {
        for (std::size_t i = 0; i < id.size(); ++i) {
            if (id.test(i) || id[i] != other[i]) {
                return i;
            }
        }
        return id.size();
    }();
    return (id.size() - 1) - leading_zeros;
}

[[nodiscard]] inline node_id generate_random_node_id() {
    Poco::Random random;
    node_id id;
    for (const auto i : std::views::iota(0uz, id.size())) {
        id.set(i, random.nextBool());
    }
    return id;
}

/**
 * @brief Routing table implementation for a Kademlia node
 * @details The routing table is composed of multiple buckets, each bucket
 *          contains a list of nodes that are close to the current node.
 *          The distance between two nodes is measured by the XOR metric.
 */
class routing_table {
    static constexpr std::size_t PARRALLEL_REQUESTS_PER_NODE_LOOKUP = 3; //!< number of maximum parrallel requests per node lookup

    enum bucket_fields {
        NODE_ID   = 0,
        ENDPOINT  = 1,
        LAST_SEEN = 2,
        METADATA  = 3
    };

    using bucket = fil::soa<                   //
        node_id,                               //!< node identifier
        endpoint,                              //!< endpoint to reach the node
        std::chrono::steady_clock::time_point, //!< last time the node was seen
        metadata_node                          //!< metadata of the node
        >;

  public:
    explicit routing_table(const node_id& id, std::size_t maximum_bucket_size)
        : id_(id)
        , maximum_bucket_size_(maximum_bucket_size) {
        for ([[maybe_unused]] auto _ : std::views::iota(0uz, id_.size())) {
            bucket new_bucket;
            new_bucket.reserve(maximum_bucket_size_);
            buckets_.push_back(std::move(new_bucket));
        }
    }

    const node_id& id() const { return id_; }

    /**
     * @param id_ep node id of the endpoint to touch
     * @param ep endpoint to touch
     * @param meta metadata of the node
     */
    void touch(const node_id& id_ep, const endpoint& ep, const metadata_node& meta) {
        const auto idx = make_bucket_idx(id_, id_ep);
        bucket& bucket = buckets_[idx];

        auto it =
            std::ranges::find_if(bucket, fil::soa_select<NODE_ID>([&](const node_id& id_in_bucket) { return id_ep == id_in_bucket; }));

        if (it == bucket.end()) {
            if (bucket.size() >= maximum_bucket_size_) {
                const auto it_lru = std::ranges::min_element(bucket, [](const auto& lhs, const auto& rhs) {
                    [[maybe_unused]] const auto& [id_lhs, ep_lhs, lastseen_lhs, meta_lhs] = lhs;
                    [[maybe_unused]] const auto& [id_rhs, ep_rhs, lastseen_rhs, meta_rhs] = rhs;
                    return lastseen_lhs < lastseen_rhs;
                });
                bucket.erase(it_lru.operator*().struct_id());
            }

            bucket.insert(id_ep, ep, std::chrono::steady_clock::now(), meta);
        } else {
            auto soa_struct           = *it;
            get<METADATA>(soa_struct) = meta;
        }
    }

    template<value_policy ValuePolicy> void find_node(ValuePolicy& policy, const node_id& node) {
        // thread_pool_.start([]() {}, "find_node");
    }

  private:
    node_id id_;                      //!< node identifier
    std::size_t maximum_bucket_size_; //!< maximum size of a bucket
    std::vector<bucket> buckets_;     //!< list of buckets
};

struct pending_request {
    std::chrono::steady_clock::time_point timestamp; //!< timestamp of the request
    std::chrono::milliseconds timeout;               //!< timeout of the request
    std::promise<std::vector<std::uint8_t>> promise; //!< promise to set the result of the request
};

/**
 *
 */
class kademlia_protocol {
    static constexpr long TIME_BETWEEN_CLEANUP =
        std::chrono::milliseconds {std::chrono::seconds {30}}.count(); //!< time interval to clean up the routing table

  public:
    explicit kademlia_protocol(const configuration& config)
        : socket_(Poco::Net::SocketAddress(config.bind_interface + std::to_string(config.listen_port)), true)
        , thread_pool_(1, config.alpha) {

        timer_.schedule( // add periodic cleanup of old requests
            Poco::Util::Timer::func([this]() {
                std::erase_if(pending_requests_, [](const auto& pair) {
                    const auto& [id, request] = pair;
                    return std::chrono::steady_clock::now() - request.timestamp > request.timeout;
                });
            }),
            TIME_BETWEEN_CLEANUP, TIME_BETWEEN_CLEANUP);

        timer_.schedule( // add periodic refresh of the routing table
            Poco::Util::Timer::func([]() {
                // refresh
            }),
            config.refresh_time.count(), config.refresh_time.count());

        socket_.setBlocking(false);
        reactor_.addEventHandler( //
            socket_, Poco::NObserver<kademlia_protocol, Poco::Net::ReadableNotification>(*this, &kademlia_protocol::on_readable));
    }

    void start() { reactor_.run(); }

  private:
    void on_readable(const Poco::AutoPtr<Poco::Net::ReadableNotification>&) {
        
    }

  private:
    std::atomic_uint64_t current_message_id_ {0};                         //!< current message id to use for the next message
    std::unordered_map<std::uint64_t, pending_request> pending_requests_; //!< map of pending requests

    Poco::Net::DatagramSocket socket_;                                    //!< UDP socket used for communication
    Poco::Net::SocketReactor reactor_;                                    //!< socket reactor to handle incoming messages
    Poco::Util::Timer timer_;                                             //!< timer to handle periodic tasks
    Poco::ThreadPool thread_pool_;                                        //!< thread pool to handle parallel requests
};

} // namespace details

/**
 * @brief Kademlia node implementation
 * @tparam ValuePolicy Policy to detect a key and retrieve the proper result
 */
template<details::value_policy ValuePolicy> class node {
  public:
    explicit node(configuration config, ValuePolicy&& policy = {});

    /**
     * @brief start the run loop of the node
     * This function is blocking and should be called in a separate thread
     */
    void start_run_loop();

    /**
     * @brief stop the run loop of the node
     * This function is non-blocking and will return immediately
     */
    void stop_run_loop();

    /**
     * @return node identifier
     */
    const node_id& id() const { return rt_.id(); }

  private:
    configuration config_;                //!< configuration of the node
    ValuePolicy policy_retriever_;        //!< policy to use to retrieve the values associated to a key

    details::routing_table rt_;           //!< routing table of the node
    details::kademlia_protocol protocol_; //!< transport layer to handle messages (UDP for kademlia protocol, TCP for value transfer)
};

template<details::value_policy KeyPolicy>
node<KeyPolicy>::node(configuration config, KeyPolicy&& policy)
    : config_(std::move(config))
    , policy_retriever_(std::forward<KeyPolicy>(policy))
    , rt_(details::generate_random_node_id(), config_.bucket_size)
    , protocol_(config_) {}

template<details::value_policy KeyPolicy> void node<KeyPolicy>::start_run_loop() {}

template<details::value_policy KeyPolicy> void node<KeyPolicy>::stop_run_loop() {}

} // namespace fil::p2p::kademlia

#endif // FIL_NODE_HH

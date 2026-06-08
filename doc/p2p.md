# P2P (Kademlia)

FiL provides an implementation of the Kademlia distributed hash table (DHT) protocol, as described in
the [original paper](https://cs.nyu.edu/~anirudh/CSCI-GA.2620-001/papers/kademlia.pdf).

## Key Features

- **XOR Metric**: Distance between nodes is calculated using the XOR of their 160-bit IDs.
- **K-Buckets**: Routing table organized into buckets of size `k`, automatically updated based on node activity.
- **Parallel Lookups**: Support for parallel node lookups using the `alpha` parameter.
- **Policy-based Storage**: Extensible value retrieval through a `ValuePolicy` concept.
- **UDP-based**: Uses UDP for lightweight communication between nodes.

## Basic Usage

### Configuration

You can configure the Kademlia node using the `fil::p2p::kademlia::configuration` struct.

```cpp
#include <fil/p2p/kademlia_node.hh>

fil::p2p::kademlia::configuration config;
config.bucket_size = 20;            // k
config.alpha = 3;                  // Parallelism
config.listen_port = 4242;
config.bind_interface = "0.0.0.0";
```

### Value Policy

To store and retrieve values in the DHT, you need to provide a `ValuePolicy`. This policy determines how keys are mapped
to values.

```cpp
struct MyValuePolicy {
    std::vector<std::uint8_t> get_value(const fil::p2p::kademlia::data_key& key) const {
        // Retrieve value for the given key
        return { 0x01, 0x02, 0x03 };
    }
};
```

### Creating and Starting a Node

```cpp
#include <fil/p2p/kademlia_node.hh>

int main() {
    fil::p2p::kademlia::configuration config;
    // ... setup config ...

    fil::p2p::kademlia::kademlia_node node(config, MyValuePolicy{});

    // Start the node's run loop (blocking)
    node.start_run_loop();

    return 0;
}
```

## Dependencies

The P2P module depends on the **POCO C++ Libraries** (specifically `PocoNet` and `PocoUtil`) for networking and event
handling.

## Implementation Details

- **Node IDs**: 160-bit identifiers (`std::bitset<160>`).
- **Routing Table**: Uses `fil::soa` for cache-friendly storage of node information in k-buckets.
- **Backtracking/Retry**: Handles timeouts and unresponsive nodes by evicting them from buckets.

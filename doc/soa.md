# Structure of Arrays (SOA)

`fil::soa` is a cache-friendly data structure that stores members of a structure in separate contiguous arrays. This is
particularly useful for Entity Component Systems (ECS) and high-performance data processing where only a subset of
members is accessed during iteration.

## Features

- **Cache-friendly**: Improves memory locality by grouping similar data together.
- **ID-based Access**: Stable `struct_id` for accessing elements, even after deletions (uses generational IDs).
- **Standard Compatible**: Supports C++ standard iterators and structured bindings.
- **Efficient Deletions**: O(1) deletions using the stable ID.

## Basic Usage

```cpp
#include <fil/datastructure/soa.hh>
#include <string>
#include <iostream>

struct Position { float x, y; };
struct Velocity { float dx, dy; };

int main() {
    // 1. Define an SOA with component types
    fil::soa<Position, Velocity> entities;

    // 2. Insert an entity and get its stable ID
    auto id = entities.insert(Position{0.0f, 0.0f}, Velocity{1.0f, 1.0f});

    // 3. Access by ID
    auto entity = entities[id];
    
    // Structured binding support
    auto& [pos, vel] = entity;
    pos.x = 10.0f;

    // 4. Iterate over all entities
    // Only 'p' and 'v' are accessed, maximizing cache efficiency
    for (auto& [p, v] : entities) {
        p.x += v.dx;
        p.y += v.dy;
    }

    // 5. Remove an entity
    entities.erase(id);

    return 0;
}
```

## Internal Details

The implementation is strongly inspired by [columnist](https://github.com/rollbear/columnist) from Björn Fahller.

Instead of an Array of Structures (AoS):
`[ {x,y,dx,dy}, {x,y,dx,dy}, {x,y,dx,dy} ]`

It uses a Structure of Arrays (SoA):

- `x:  [x1, x2, x3]`
- `y:  [y1, y2, y3]`
- `dx: [dx1, dx2, dx3]`
- `dy: [dy1, dy2, dy3]`

This layout ensures that when you iterate over just `x` and `y`, the CPU only loads relevant data into the cache,
avoiding "polluting" it with `dx` and `dy` data.

### Generational IDs

The `struct_id` used by `fil::soa` contains both an index and a generation counter. This prevents "stale" IDs from
accessing new elements that might have been allocated at the same index after a deletion.

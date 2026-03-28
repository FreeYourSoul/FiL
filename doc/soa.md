# Structure of Arrays (SOA)

`fil::soa` is a data structure inspired by the ECS (Entity Component System) architecture. It stores data in a way that
is cache-friendly and allows for efficient iteration over data.

## Features

- Cache-friendly memory layout (Structure of Arrays).
- Compatible with C++ standard iterators and algorithms.
- ID-based access to elements.
- Support for structured bindings.

## Basic Usage

```cpp
#include <fil/datastructure/soa.hh>
#include <string>
#include <iostream>

struct Position { float x, y; };
struct Velocity { float dx, dy; };

int main() {
    // Define an SOA with two components
    fil::soa<Position, Velocity> entities;

    // Insert an entity
    auto id = entities.insert(Position{0, 0}, Velocity{1, 1});

    // Access by ID
    auto entity = entities[id];
    // Structured binding support
    auto& [pos, vel] = entity;
    pos.x = 10.0f;

    // Iterate over all entities
    for (auto& [p, v] : entities) {
        p.x += v.dx;
        p.y += v.dy;
    }

    return 0;
}
```

## Internal Details

The implementation is strongly inspired by [columnist](https://github.com/rollbear/columnist) from Björn Fahller.
Instead of an array of structures (AoS), it stores each member of the structure in a separate contiguous array (SoA),
which significantly improves cache locality when only a subset of members is accessed during iteration.

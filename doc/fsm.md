# Finite State Machine (FSM)

FiL includes a simple, header-only implementation of a Finite State Machine.

## Requirements

The FSM requires:

- Transition definitions from one state to another.
- Handlers to determine when a state transition is possible.
- `on_entry` state actions (optional).
- `advance` method usage to trigger the state machine transitions.

## Basic Usage

```cpp
#include <fil/fsm/state_machine.hh>

enum class State {
    OFF,
    ON,
    ERROR
};

// Define your FSM
fil::state_machine<State> fsm(State::OFF);

// Add transitions: from OFF to ON, always possible (true)
fsm.add_transition(State::OFF, State::ON, []() { return true; });

// Add on_entry callback
fsm.on_entry(State::ON, []() {
    std::cout << "Turned on!" << std::endl;
});

// Trigger a transition check
fsm.advance(); // This will call the handlers and move state if possible
```

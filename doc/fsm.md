# Finite State Machine (FSM)

FiL includes a simple, header-only implementation of a Finite State Machine (FSM). It allows you to define states,
transitions between them, and actions to be performed when entering a state.

## Core Concepts

- **State**: An enumeration representing the possible conditions of the machine.
- **Transition**: A rule that moves the machine from one state to another if a specific condition (handler) is met.
- **Entry Action**: A callback executed when the machine enters a specific state.
- **Advance**: The process of checking all possible transitions from the current state and moving to a new state if a
  transition handler returns `true`.

## Basic Usage

### Example: A Simple Light Switch

```cpp
#include <fil/fsm/state_machine.hh>
#include <iostream>

enum class LightState {
    OFF,
    ON,
    ERROR
};

int main() {
    // 1. Initialize the FSM with the starting state
    fil::state_machine<LightState> fsm(LightState::OFF);

    // 2. Define transitions
    // From OFF to ON, only possible if it's not "night" (simulated)
    bool is_day = true;
    fsm.add_transition(LightState::OFF, LightState::ON, [&is_day]() { 
        return is_day; 
    });

    // From ON to OFF, always possible
    fsm.add_transition(LightState::ON, LightState::OFF, []() { 
        return true; 
    });

    // 3. Define entry actions
    fsm.on_entry(LightState::ON, []() {
        std::cout << "Light is now ON" << std::endl;
    });

    fsm.on_entry(LightState::OFF, []() {
        std::cout << "Light is now OFF" << std::endl;
    });

    // 4. Run the machine
    std::cout << "Initial state: OFF" << std::endl;
    
    fsm.advance(); // Checks transitions from OFF. Since is_day is true, moves to ON.
    // Output: "Light is now ON"

    fsm.advance(); // Checks transitions from ON. Moves to OFF.
    // Output: "Light is now OFF"

    return 0;
}
```

## Advanced Usage

### Transition Priority

Transitions are checked in the order they are added. The first transition whose handler returns `true` will be taken.

### Manual State Management

While `advance()` is the preferred way to move between states based on rules, you can also check the current state using
`fsm.current_state()`.

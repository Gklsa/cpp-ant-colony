# Ant Colony Simulation · C++ Coursework

Interactive ant-colony simulation with agent states, a grid environment and an SFML/ImGui control panel.

## Code map

- `src/Ant.cpp` / `Ant.hpp`: searching/carrying states, scout/forager roles and movement logic.
- `src/Environment.cpp` / `Environment.hpp`: food, obstacles, pheromones and grid connectivity.
- `src/Simulation.cpp` / `Simulation.hpp`: simulation loop, rendering and adjustable GUI settings.
- `src/main.cpp`: application entry; `CMakeLists.txt`: C++17 build and dependency declarations.

## Build

With CMake and a C++17 compiler:

```bash
cmake -S . -B build
cmake --build build --config Release
```

Run the generated `AntColonySimulation` executable. CMake retains FetchContent declarations for SFML, ImGui and ImGui-SFML; these dependencies must be available locally or obtained during a later configure step.

Coursework snapshot with original source attribution retained; team/starter provenance has not been fully separated, and no new blanket license is added. No dependencies or compiled output are included. See `EXCLUDED.md`.

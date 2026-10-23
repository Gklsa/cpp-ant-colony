# Ant Colony Simulation

A C++17 coursework project that simulates ants searching for food and returning to a nest. SFML draws the environment and agents; an ImGui panel controls the simulation.

Ants switch between searching and carrying states. Returning ants deposit pheromones, searching ants choose directions using pheromone weights, and trails evaporate over time. Scouts and foragers use different behavior weights; their roles are assigned when the population is created.

## Project structure

```text
src/
  main.cpp             Application entry point
  Simulation.hpp/cpp   Window, simulation loop, rendering and controls
  Ant.hpp/cpp          Search/carry states, role modifiers and movement
  Environment.hpp/cpp  Terrain, nest, food and pheromone grid
CMakeLists.txt         Build target and dependency versions
```

## Build and run on Windows

Install these tools first:

- Visual Studio 2019 or Build Tools 2019 with **Desktop development with C++**, the MSVC compiler and a Windows SDK.
- **CMake 3.14 or newer** and **Git**, both available in your terminal.
- Internet access for the first configuration to fetch the dependencies.

CMake fetches SFML **2.6.1**, Dear ImGui **v1.90.4-docking** and ImGui-SFML **v2.6**. You do not need to install those libraries separately.

Open PowerShell in the project folder containing `CMakeLists.txt`, then run:

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
.\build\Release\AntColonySimulation.exe
```

Run the build command again after changing source files. If `build` was configured with a different generator, use another directory name consistently in all three commands.

For a single-configuration generator such as Ninja, set `-DCMAKE_BUILD_TYPE=Release` when configuring; its executable is directly in the build directory rather than a `Release` subfolder. Ninja also requires its build tool and an initialized compiler environment.

## Use the simulation

The application opens stopped. Choose the initial settings, then click **START SIMULATION**.

- **STOP SIMULATION** freezes movement and pheromone evaporation.
- Clicking **START SIMULATION** again creates a new terrain and population; it does not resume the previous run.
- Close the window to finish. The console then asks you to press **Enter** to exit.

| Control | Effect and when it applies |
|---|---|
| Ant Population | 10–1,000 ants. Changing it recreates the entire population at the nest, including during a run. |
| Scout Ratio | Fraction assigned as scouts. Applies when the population is recreated or a new run starts. |
| Movement Speed | Changes movement speed during a run. |
| Random Chaos | Changes heading jitter when ants return home. It has limited effect on ordinary searching in the current implementation. |
| Pheromone Evaporation | Changes the rate at which trails fade during a run. |
| Food Sources | Requested number of food cells. Applies on the next **START SIMULATION**. |
| Food per Cell | Initial food capacity of each source. Applies on the next **START SIMULATION**. |

Colors: **yellow** scouts, **white** foragers, **blue** nest, **green** food, **brown** obstacles and **purple** pheromone trails.

Food is depleted as ants pick it up. Carrying ants head toward the nest while avoiding obstacles, then return to searching. Both roles can collect food; the current model does not dynamically allocate tasks or report an optimal path or collected-food total.

The window is fixed at **1700 × 1000 pixels**; use a display configuration that can show the control panel.

## Background references

- F. Brioccia, [Swarm intelligence: Inside the ant colony](https://towardsdatascience.com/swarm-intelligence-inside-the-ant-colony-9ffbce22a736) (2022).
- H. Sayama, *Introduction to the Modeling and Analysis of Complex Systems*, Open SUNY Textbooks (2015).
- Sebastian Lague, [Coding Adventure: Ant and Slime Simulations](https://www.youtube.com/watch?v=X-iSQQgOd1A) (2021).

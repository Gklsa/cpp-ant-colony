#pragma once

#include "Environment.hpp"
#include "Ant.hpp"

#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

// simulation parameters (adjustable via GUI)
struct SimulationSettings {
    int   antCount        = 200;
    float scoutRatio      = 0.25f; // 25% scouts,rest foragers

    int   foodCount       = 150;
    float foodPerCell     = 8.0f;

    float evaporationRate = 0.35f;
    float sensorAngle     = 40.0f;
    float sensorDistance  = 20.0f;

    float moveSpeed       = 2.0f;
    float randomness      = 0.25f;

    bool  isRunning       = false;
};

// main simulation class: manages environment, ants, and rendering
class Simulation {
public:
    Simulation(int width, int height);
    ~Simulation();

    // main game loop
    void run();

private:
    void processEvents();      // handle window events
    void update(sf::Time dt);  // update simulation state
    void render();             // render simulation and UI
    void renderUI();           // render ImGui control panel
    void spawnAnts();          // create ants at home position

    std::unique_ptr<Environment> environment;
    std::vector<std::unique_ptr<Ant>> ants;
    
    sf::RenderWindow window;
    sf::Clock        deltaClock;
    int   gridWidth  = 0;
    int   gridHeight = 0;
    float cellSize   = 6.0f;

    static constexpr int SIM_WIDTH    = 1200;
    static constexpr int SIM_HEIGHT   = 1000;
    static constexpr int PANEL_WIDTH  = 500;
    
    static constexpr int WINDOW_WIDTH  = SIM_WIDTH + PANEL_WIDTH;
    static constexpr int WINDOW_HEIGHT = SIM_HEIGHT;

    SimulationSettings settings;
    float elapsedTime        = 0.0f;
    int   totalFoodCollected = 0;
};
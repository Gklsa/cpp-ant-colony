#pragma once

#include <vector>
#include <iostream>
#include <random>
#include <algorithm>

// cell types in the grid
enum class CellType {
    EMPTY,
    WALL,
    FOOD,
    HOME,
    OBSTACLE
};

// single grid cell containing type, food, and pheromone
struct Cell {
    CellType type = CellType::EMPTY;
    float foodAmount = 0.0f;
    float pheromone = 0.0f;
};

// grid-based environment for ant colony simulation
class Environment {
public:
    Environment(int width, int height);
    ~Environment() = default;

    // generate terrain with obstacles and food
    void initialize(int foodCount, int obstacleCount = 0, float foodCapacity = 5.0f);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    CellType getCell(int x, int y) const;
    const Cell& getCellData(int x, int y) const;
    void setCell(int x, int y, CellType type);
    bool isValid(int x, int y) const;

    // food management
    bool hasFood(int x, int y) const;
    void takeFood(int x, int y, float amount);
    float getFoodAmount(int x, int y) const;
    float getMaxFoodPerCell() const { return maxFoodPerCell; }

    // pheromone management
    float getPheromone(int x, int y) const;
    void addPheromone(int x, int y, float amount);
    void evaporatePheromones(float rho, float dt);

    std::pair<int,int> getHomeCell() const { return {homeX, homeY}; }

private:
    int width;
    int height;

    int homeX;
    int homeY;

    float maxPheromone = 5.0f;
    float maxFoodPerCell = 5.0f;

    std::vector<std::vector<Cell>> grid;

    // initialization helpers
    void clearGrid();
    void buildBoundaryWalls();
    void placeHomeCenter();
    void generateNoiseObstacles(std::mt19937& rng, double threshold);
    void smoothObstacles(int iterations);
    void addExtraObstacles(std::mt19937& rng, int obstacleScale);
    void carveCorridors(std::mt19937& rng);
    void clearHomeArea();
    void ensureConnectivity();
    void placeFood(std::mt19937& rng, int foodCount);
    void resetPheromones();
};


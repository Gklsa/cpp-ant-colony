#pragma once

#include "Environment.hpp"
#include <random>
#include <cmath>

// Ant agent with state machine (searching for food or carrying food home)
class Ant {
public:
    // 2D position with floating point precision
    struct Position {
        float x;
        float y;
        bool operator==(const Position& other) const;
    };

    enum class State {
        Searching,  // looking for food
        Carrying    // carrying food back to home
    };
    enum class Role {
        Scout,      // exploring the environment and discovering new food sources
        Forager     // collecting food and transporting back to the nest
    };

    Ant(float startX, float startY, Role role = Role::Forager);
    ~Ant() = default;

    // updates ant position and behavior based on environment
    void move(Environment& env, float dt, float speed, float randomness);  
    
    const Position& getPosition() const;

    State getState() const;
    
    Role getRole() const;

    void setRole(Role r);


private:
    Position position{}; 
    float angle = 0.0f;       // movement direction in radians
    State state = State::Searching;
    Role role = Role::Forager;

    // thread-local random number generator
    static std::mt19937& getRNG();

    // movement helpers
    void applyRoleModifiers(float baseRandomness,
                            float& roleRandomness,
                            float& pheromoneWeightScale) const;
    void handleSearching(Environment& env,
                         int cellX,
                         int cellY,
                         float roleRandomness,
                         float pheromoneWeightScale);
    void handleCarrying(Environment& env,
                        int cellX,
                        int cellY,
                        float roleRandomness);
    void applyMovement(Environment& env, float dt, float speed);
    void resolveBoundary(Environment& env, float moveDist, float& newX, float& newY);
    void resolveObstacle(Environment& env, float moveDist, float& newX, float& newY);
};

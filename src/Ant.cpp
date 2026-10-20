#include "Ant.hpp"
#include <cmath>
#include <ctime>
#include <algorithm>
#include <random>
#include <array>

constexpr float PI        = 3.14159265f;
constexpr float CELL_SIZE = 6.0f; // must match Simulation

// thread-local RNG for each ant，RNG is a random number generator
std::mt19937& Ant::getRNG() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

// initialize ant at starting position with random direction
Ant::Ant(float startX, float startY, Role r) 
    : position{startX, startY},
      angle(0.0f),
      state(State::Searching),
      role(r) {

    std::uniform_real_distribution<float> dist(0.0f, 2.0f * PI);
    angle = dist(getRNG());
}

const Ant::Position& Ant::getPosition() const {
    return position;
}

bool Ant::Position::operator==(const Position& other) const {
    constexpr float epsilon = 0.001f;
    return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon;
}

Ant::State Ant::getState() const {
    return state;
}

Ant::Role Ant::getRole() const {
    return role;
}

void Ant::setRole(Role r) {
    role = r;
}

// main movement logic: state machine for searching/carrying behavior
void Ant::move(Environment& env, float dt, float speed, float randomness) {
    int cellX = static_cast<int>(position.x / CELL_SIZE);
    int cellY = static_cast<int>(position.y / CELL_SIZE);

    float roleRandomness = randomness;
    float pheromoneWeightScale = 1.0f;
    applyRoleModifiers(randomness, roleRandomness, pheromoneWeightScale);

    if (state == State::Searching) {
        handleSearching(env, cellX, cellY, roleRandomness, pheromoneWeightScale);
    } else {
        handleCarrying(env, cellX, cellY, roleRandomness);
    }

    applyMovement(env, dt, speed);
}

void Ant::applyRoleModifiers(float baseRandomness,
                             float& roleRandomness,
                             float& pheromoneWeightScale) const {
    roleRandomness = baseRandomness;
    pheromoneWeightScale = 1.0f;

    if (role == Role::Scout) {
        roleRandomness *= 2.0f;
        pheromoneWeightScale *= 0.3f;
    } else { // Forager
        roleRandomness *= 0.7f;
        pheromoneWeightScale *= 1.5f;
    }
}

void Ant::handleSearching(Environment& env,
                          int cellX,
                          int cellY,
                          float roleRandomness,
                          float pheromoneWeightScale) {
    // found food: pick it up and head home
    if (env.hasFood(cellX, cellY)) {
        env.takeFood(cellX, cellY, 1.0f);
        state = State::Carrying;

        auto [homeCx, homeCy] = env.getHomeCell();
        float homeX = (homeCx + 0.5f) * CELL_SIZE;
        float homeY = (homeCy + 0.5f) * CELL_SIZE;

        float dx = homeX - position.x;
        float dy = homeY - position.y;
        angle = std::atan2(dy, dx);
        return;
    }

    // no food: follow pheromone trails or random walk
    struct Option {
        float angleOffset;
        float weight;
    };

    // check three directions: forward, left, right
    std::array<Option, 3> options{{
        { 0.0f,       0.0f },
        { +PI / 4.0f, 0.0f },
        { -PI / 4.0f, 0.0f }
    }};

    const float epsilon = 0.01f;
    const float beta    = 2.0f;

    float sumWeight = 0.0f;

    // calculate weight for each direction based on pheromone concentration
    for (auto& opt : options) {
        float dirAngle = angle + opt.angleOffset;

        float sampleX = position.x + std::cos(dirAngle) * CELL_SIZE;
        float sampleY = position.y + std::sin(dirAngle) * CELL_SIZE;

        int sx = static_cast<int>(sampleX / CELL_SIZE);
        int sy = static_cast<int>(sampleY / CELL_SIZE);

        CellType cellType = env.getCell(sx, sy);
        if (cellType == CellType::WALL || cellType == CellType::OBSTACLE) {
            opt.weight = 0.0f;
            continue;
        }

        float pher = env.getPheromone(sx, sy) * pheromoneWeightScale;
        opt.weight = std::pow(pher + epsilon, beta);
        sumWeight += opt.weight;
    }

    // choose direction probabilistically based on weights
    if (sumWeight > 0.0f) {
        std::uniform_real_distribution<float> dist(0.0f, sumWeight);
        float r = dist(getRNG());

        for (auto& opt : options) {
            if (r <= opt.weight) {
                angle += opt.angleOffset;
                break;
            }
            r -= opt.weight;
        }
    } else {
        // no pheromone: random walk
        std::uniform_real_distribution<float> angleDist(-roleRandomness, roleRandomness);
        angle += angleDist(getRNG());
    }
}

void Ant::handleCarrying(Environment& env,
                         int cellX,
                         int cellY,
                         float roleRandomness) {
    // carrying food: drop pheromone and head home
    CellType t = env.getCell(cellX, cellY);

    if (t != CellType::HOME && t != CellType::WALL) {
        env.addPheromone(cellX, cellY, 0.4f);
    }

    auto [homeCx, homeCy] = env.getHomeCell();
    if (cellX == homeCx && cellY == homeCy) {
        // reached home: drop food and start searching again
        state = State::Searching;
        std::uniform_real_distribution<float> dist(0.0f, 2.0f * PI);
        angle = dist(getRNG());
    } else {
        // move toward home with slight randomness
        float homeX = (homeCx + 0.5f) * CELL_SIZE;
        float homeY = (homeCy + 0.5f) * CELL_SIZE;

        float dx = homeX - position.x;
        float dy = homeY - position.y;
        angle = std::atan2(dy, dx);

        std::uniform_real_distribution<float> jitter(-roleRandomness * 0.2f,
                                                     roleRandomness * 0.2f);
        angle += jitter(getRNG());
    }
}

void Ant::applyMovement(Environment& env, float dt, float speed) {
    float moveDist = speed * CELL_SIZE * dt;
    float newX = position.x + std::cos(angle) * moveDist;
    float newY = position.y + std::sin(angle) * moveDist;

    resolveBoundary(env, moveDist, newX, newY);
    resolveObstacle(env, moveDist, newX, newY);

    position.x = newX;
    position.y = newY;
}

void Ant::resolveBoundary(Environment& env, float moveDist, float& newX, float& newY) {
    int width  = env.getWidth();
    int height = env.getHeight();
    float limitX = static_cast<float>(width  * CELL_SIZE);
    float limitY = static_cast<float>(height * CELL_SIZE);

    if (newX <= 0.0f || newX >= limitX) {
        angle = PI - angle;
        newX = std::clamp(newX, 0.0f, limitX);
    }
    if (newY <= 0.0f || newY >= limitY) {
        angle = -angle;
        newY = std::clamp(newY, 0.0f, limitY);
    }
}

void Ant::resolveObstacle(Environment& env, float moveDist, float& newX, float& newY) {
    float limitX = static_cast<float>(env.getWidth()  * CELL_SIZE);
    float limitY = static_cast<float>(env.getHeight() * CELL_SIZE);

    int newCellX = static_cast<int>(newX / CELL_SIZE);
    int newCellY = static_cast<int>(newY / CELL_SIZE);
    CellType dest = env.getCell(newCellX, newCellY);

    if (dest != CellType::WALL && dest != CellType::OBSTACLE) {
        return;
    }

    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> turnDist(-PI * 0.5f, PI * 0.5f);

    bool foundFreeCell = false;

    for (int attempt = 0; attempt < 5; ++attempt) {
        float tryAngle = angle + turnDist(rng);
        float tryX = position.x + std::cos(tryAngle) * moveDist;
        float tryY = position.y + std::sin(tryAngle) * moveDist;

        float clampedX = std::clamp(tryX, 0.0f, limitX);
        float clampedY = std::clamp(tryY, 0.0f, limitY);

        int tryCellX = static_cast<int>(clampedX / CELL_SIZE);
        int tryCellY = static_cast<int>(clampedY / CELL_SIZE);
        CellType tryDest = env.getCell(tryCellX, tryCellY);

        if (tryDest != CellType::WALL && tryDest != CellType::OBSTACLE) {
            angle = tryAngle;
            newX  = clampedX;
            newY  = clampedY;
            foundFreeCell = true;
            break;
        }
    }

    if (!foundFreeCell) {
        newX = position.x;
        newY = position.y;
    }
}


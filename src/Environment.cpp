#include "Environment.hpp"

#include <random>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <queue>

namespace {

// smooth interpolation function
inline double fade(double t) {
    return t * t * (3.0 - 2.0 * t);
}

// linear interpolation
inline double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// hash function for noise generation
inline double hashNoise(int x, int y, unsigned int seed) {
    unsigned int h = seed;
    h ^= x * 374761393u;
    h ^= y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return (h & 0xFFFFFFu) / 16777215.0;
}

// bilinear interpolation for smooth 2D noise
double smoothNoise2D(double x, double y, unsigned int seed) {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    double fx = x - x0;
    double fy = y - y0;

    double v00 = hashNoise(x0, y0, seed);
    double v10 = hashNoise(x1, y0, seed);
    double v01 = hashNoise(x0, y1, seed);
    double v11 = hashNoise(x1, y1, seed);

    double ix0 = lerp(v00, v10, fade(fx));
    double ix1 = lerp(v01, v11, fade(fx));
    return lerp(ix0, ix1, fade(fy));
}

// multi-octave Perlin-style noise
double perlin2D(double x, double y, unsigned int seed) {
    double freq = 0.05;
    double amp  = 1.0;
    double sum  = 0.0;
    double norm = 0.0;

    for (int octave = 0; octave < 3; ++octave) {
        double n = smoothNoise2D(x * freq, y * freq, seed + octave * 1337u);
        sum  += n * amp;
        norm += amp;
        amp  *= 0.5;
        freq *= 2.0;
    }

    return sum / norm;
}

}

// create empty environment grid
Environment::Environment(int w, int h)
    : width(w),
      height(h),
      homeX(w / 2),
      homeY(h / 2),
      maxPheromone(5.0f),
      maxFoodPerCell(5.0f)
{
    grid.assign(height, std::vector<Cell>(width));
}

// generate terrain: walls, obstacles, food, and corridors
void Environment::initialize(int foodCount, int obstacleScale, float foodCapacity)
{
    clearGrid();
    maxFoodPerCell = foodCapacity;
    buildBoundaryWalls();
    placeHomeCenter();

    std::random_device rd;
    unsigned int seed = rd();
    std::mt19937 rng(seed);

    constexpr double obstacleNoiseThreshold = 0.62;
    generateNoiseObstacles(rng, obstacleNoiseThreshold);

    constexpr int smoothIterations = 3;
    smoothObstacles(smoothIterations);

    addExtraObstacles(rng, obstacleScale);
    carveCorridors(rng);
    clearHomeArea();
    ensureConnectivity();
    placeFood(rng, foodCount);
    resetPheromones();
}

void Environment::clearGrid() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x] = Cell{};
        }
    }
}

void Environment::buildBoundaryWalls() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                grid[y][x].type = CellType::WALL;
            }
        }
    }
}

void Environment::placeHomeCenter() {
    homeX = width / 2;
    homeY = height / 2;
    grid[homeY][homeX].type = CellType::HOME;
}

void Environment::generateNoiseObstacles(std::mt19937& rng, double threshold) {
    unsigned int seed = rng(); // derive from provided rng for reproducibility
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (x == homeX && y == homeY) continue;

            double n = perlin2D(static_cast<double>(x),
                                static_cast<double>(y),
                                seed);

            if (n > threshold) {
                grid[y][x].type = CellType::OBSTACLE;
            } else {
                grid[y][x].type = CellType::EMPTY;
            }
        }
    }
}

void Environment::smoothObstacles(int iterations) {
    auto countObstacleNeighbors = [&](int cx, int cy) {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = cx + dx;
                int ny = cy + dy;
                if (nx <= 0 || nx >= width - 1 || ny <= 0 || ny >= height - 1)
                    continue;
                CellType t = grid[ny][nx].type;
                if (t == CellType::OBSTACLE || t == CellType::WALL) {
                    ++count;
                }
            }
        }
        return count;
    };

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<std::vector<CellType>> newTypes(
            height, std::vector<CellType>(width, CellType::EMPTY));

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {

                CellType current = grid[y][x].type;

                // preserve walls and home
                if (current == CellType::WALL ||
                    (x == homeX && y == homeY)) {
                    newTypes[y][x] = current;
                    continue;
                }

                int neighbors = countObstacleNeighbors(x, y);

                // CA rules for cave generation
                if (neighbors >= 5) {
                    newTypes[y][x] = CellType::OBSTACLE;  
                } else if (neighbors <= 2) {
                    newTypes[y][x] = CellType::EMPTY;     
                } else {
                    newTypes[y][x] = current;
                }
            }
        }

        // apply changes
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                grid[y][x].type = newTypes[y][x];
            }
        }
    }
}

void Environment::addExtraObstacles(std::mt19937& rng, int obstacleScale) {
    std::uniform_int_distribution<int> distX(1, width  - 2);
    std::uniform_int_distribution<int> distY(1, height - 2);

    int extraSingles = obstacleScale * 40;
    int placed = 0;
    int tries  = 0;
    int maxTries = extraSingles * 5;

    while (placed < extraSingles && tries < maxTries) {
        int x = distX(rng);
        int y = distY(rng);
        ++tries;

        Cell& c = grid[y][x];

        // only place on empty cells
        if (c.type == CellType::EMPTY) {
            c.type = CellType::OBSTACLE;
            ++placed;
        }
    }
}

void Environment::carveCorridors(std::mt19937& rng) {
    std::uniform_int_distribution<int> distEdgeX(1, width  - 2);
    std::uniform_int_distribution<int> distEdgeY(1, height - 2);

    auto carveWigglyCorridor = [&](int x0, int y0, int x1, int y1)
    {
        int x = x0;
        int y = y0;

        int maxSteps = width + height;

        std::uniform_real_distribution<float> prob01(0.0f, 1.0f);
        std::uniform_int_distribution<int> randDir(0, 3);

        // dig a 5x5 area at current position
        auto digAt = [&](int cx, int cy)
        {
            if (cx <= 0 || cx >= width - 1 || cy <= 0 || cy >= height - 1)
                return;
            if (cx == homeX && cy == homeY)
                return;

            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx <= 0 || nx >= width - 1 || ny <= 0 || ny >= height - 1)
                        continue;
                    if (nx == homeX && ny == homeY)
                        continue;

                    grid[ny][nx].type       = CellType::EMPTY;
                    grid[ny][nx].foodAmount = 0.0f;
                }
            }
        };

        // move toward target with 70% probability, random turn with 30%
        while (maxSteps-- > 0) {
            digAt(x, y);

            // stop when close to target
            if (std::abs(x - x1) <= 2 && std::abs(y - y1) <= 2)
                break;

            int dirX = 0;
            int dirY = 0;

            if (prob01(rng) < 0.7f) {
                // move toward target
                if (x < x1) dirX = 1;
                else if (x > x1) dirX = -1;

                if (y < y1) dirY = 1;
                else if (y > y1) dirY = -1;

                // prefer horizontal or vertical movement (not diagonal)
                if (prob01(rng) < 0.6f) {
                    if (std::abs(x - x1) > std::abs(y - y1))
                        dirY = 0;
                    else
                        dirX = 0;
                }
            } else {
                // random turn for wiggly effect
                switch (randDir(rng)) {
                    case 0: dirY = -1; break;  // up
                    case 1: dirY =  1; break; // down
                    case 2: dirX = -1; break; // left
                    case 3: dirX =  1; break; // right
                }
            }

            if (dirX == 0 && dirY == 0)
                continue;

            x += dirX;
            y += dirY;

            // keep within bounds
            if (x <= 1)        x = 1;
            if (x >= width-2)  x = width-2;
            if (y <= 1)        y = 1;
            if (y >= height-2) y = height-2;
        }
    };

    // carve 4 corridors: top, bottom, left, right edges
    carveWigglyCorridor(homeX, homeY, distEdgeX(rng), 1);            // to top edge
    carveWigglyCorridor(homeX, homeY, distEdgeX(rng), height - 2);   // to bottom edge
    carveWigglyCorridor(homeX, homeY, 1,             distEdgeY(rng)); // to left edge
    carveWigglyCorridor(homeX, homeY, width - 2,     distEdgeY(rng)); // to right edge
}

void Environment::clearHomeArea() {
    int clearRadius = 15;

    for (int dy = -clearRadius; dy <= clearRadius; ++dy) {
        for (int dx = -clearRadius; dx <= clearRadius; ++dx) {
            int x = homeX + dx;
            int y = homeY + dy;

            if (x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1)
                continue;

            if (x == homeX && y == homeY)
                continue;

            // circular check (not square)
            if (dx * dx + dy * dy > clearRadius * clearRadius)
                continue;

            grid[y][x].type       = CellType::EMPTY;
            grid[y][x].foodAmount = 0.0f;
        }
    }
}

void Environment::ensureConnectivity() {
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));

    auto isWalkable = [&](int x, int y) -> bool {
        CellType t = grid[y][x].type;
        return (t == CellType::EMPTY || t == CellType::HOME);
    };

    std::queue<std::pair<int,int>> q;

    // BFS from home
    q.push({homeX, homeY});
    visited[homeY][homeX] = true;

    const int dx4[4] = { 1, -1, 0, 0 };
    const int dy4[4] = { 0, 0, 1, -1 };

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        // check 4 directions
        for (int dir = 0; dir < 4; ++dir) {
            int nx = cx + dx4[dir];
            int ny = cy + dy4[dir];

            if (nx <= 0 || nx >= width - 1 || ny <= 0 || ny >= height - 1)
                continue;

            if (!visited[ny][nx] && isWalkable(nx, ny)) {
                visited[ny][nx] = true;
                q.push({nx, ny});
            }
        }
    }

    // convert unreachable walkable cells to obstacles
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!visited[y][x] && isWalkable(x, y)) {
                grid[y][x].type       = CellType::OBSTACLE;
                grid[y][x].foodAmount = 0.0f;
            }
        }
    }
}

void Environment::placeFood(std::mt19937& rng, int foodCount) {
    std::uniform_int_distribution<int> distX(1, width  - 2);
    std::uniform_int_distribution<int> distY(1, height - 2);

    int placedFood = 0;
    int foodTry    = 0;
    int maxFoodTry = std::max(1, foodCount * 20);

    while (placedFood < foodCount && foodTry < maxFoodTry) {
        int x = distX(rng);
        int y = distY(rng);
        ++foodTry;

        if (grid[y][x].type == CellType::EMPTY) {
            grid[y][x].type       = CellType::FOOD;
            grid[y][x].foodAmount = maxFoodPerCell;
            ++placedFood;
        }
    }
}

void Environment::resetPheromones() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x].pheromone = 0.0f;
        }
    }
}

// get cell type at position
CellType Environment::getCell(int x, int y) const
{
    if (!isValid(x, y)) {
        return CellType::WALL;
    }
    return grid[y][x].type;
}

// get full cell data
const Cell& Environment::getCellData(int x, int y) const
{
    static Cell dummyWall{CellType::WALL, 0.0f, 0.0f};
    if (!isValid(x, y)) {
        return dummyWall;
    }
    return grid[y][x];
}

// set cell type
void Environment::setCell(int x, int y, CellType type)
{
    if (isValid(x, y)) {
        grid[y][x].type = type;
    }
}

// check if coordinates are valid
bool Environment::isValid(int x, int y) const
{
    return (x >= 0 && x < width && y >= 0 && y < height);
}

// check if cell has food
bool Environment::hasFood(int x, int y) const
{
    if (!isValid(x, y)) return false;
    const Cell& c = grid[y][x];
    return (c.type == CellType::FOOD && c.foodAmount > 0.0f);
}

// remove food from cell
void Environment::takeFood(int x, int y, float amount)
{
    if (!isValid(x, y)) return;
    Cell& c = grid[y][x];

    if (c.foodAmount <= 0.0f) return;

    c.foodAmount = std::max(0.0f, c.foodAmount - amount);

    if (c.foodAmount <= 0.0f && c.type == CellType::FOOD) {
        c.type = CellType::EMPTY;
    }
}

// get food amount at position
float Environment::getFoodAmount(int x, int y) const
{
    if (!isValid(x, y)) return 0.0f;
    return grid[y][x].foodAmount;
}

// get pheromone concentration at position
float Environment::getPheromone(int x, int y) const
{
    if (!isValid(x, y)) return 0.0f;
    return grid[y][x].pheromone;
}

// add pheromone to cell (capped at max)
void Environment::addPheromone(int x, int y, float amount)
{
    if (!isValid(x, y)) return;
    Cell& c = grid[y][x];

    c.pheromone += amount;
    if (c.pheromone > maxPheromone) {
        c.pheromone = maxPheromone;
    }
}

// evaporate pheromones over time
void Environment::evaporatePheromones(float rho, float dt)
{
    float factor = 1.0f - rho * dt;
    if (factor < 0.0f) factor = 0.0f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Cell& c = grid[y][x];
            c.pheromone *= factor;
            if (c.pheromone < 0.001f) {
                c.pheromone = 0.0f;
            }
        }
    }
}

#include "Simulation.hpp"
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>
#include <string>
#include <cstdio>

// initialize window, ImGui, environment, and ants
Simulation::Simulation(int /*w*/, int /*h*/)
    : cellSize(6.0f)
{
    gridWidth  = SIM_WIDTH  / static_cast<int>(cellSize);
    gridHeight = SIM_HEIGHT / static_cast<int>(cellSize);

    // create SFML window
    sf::ContextSettings contextSettings;
    contextSettings.antialiasingLevel = 8;
    window.create(
        sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Ant Colony Sprint 2",
        sf::Style::Close,
        contextSettings
    );
    window.setFramerateLimit(60);

    // initialize ImGui
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "ImGui Init Failed\n";
    }

    // configure ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding  = 6.0f;
    style.FrameRounding   = 4.0f;
    style.FramePadding    = ImVec2(8, 4);
    style.GrabRounding    = 4.0f;
    style.GrabMinSize     = 12.0f;
    style.ScrollbarSize   = 16.0f;
    style.WindowPadding   = ImVec2(10, 10);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.4f; 

    // create environment and initialize terrain
    environment = std::make_unique<Environment>(gridWidth, gridHeight);
    environment->initialize(settings.foodCount, 60, settings.foodPerCell);

    ants.reserve(2000);
    spawnAnts();
}

Simulation::~Simulation() {
    ImGui::SFML::Shutdown();
}

// create ants at home position
void Simulation::spawnAnts() {
    ants.clear();

    float centerX = static_cast<float>(SIM_WIDTH)  / 2.0f;
    float centerY = static_cast<float>(SIM_HEIGHT) / 2.0f;

    int scoutCount = static_cast<int>(settings.antCount * settings.scoutRatio);

    for (int i = 0; i < settings.antCount; ++i) {
        Ant::Role role = (i < scoutCount) ? Ant::Role::Scout : Ant::Role::Forager;
        ants.push_back(std::make_unique<Ant>(centerX, centerY, role));
    }
}

// main game loop
void Simulation::run() {
    while (window.isOpen()) {
        processEvents();

        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        update(dt);
        render();
    }
}

// handle window events
void Simulation::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(window, event);
        if (event.type == sf::Event::Closed)
            window.close();
    }
}

// update simulation: evaporate pheromones and move ants
void Simulation::update(sf::Time dt) {
    if (ants.size() != static_cast<size_t>(settings.antCount)) {
        spawnAnts();
    }

    if (settings.isRunning) {
        elapsedTime += dt.asSeconds();

        environment->evaporatePheromones(settings.evaporationRate,
                                         dt.asSeconds());

        for (auto& ant : ants) {
            ant->move(*environment, dt.asSeconds(),
                      settings.moveSpeed,
                      settings.randomness);
        }
    }
}

// helper: center next ImGui item
static void CenterNextItem(float itemWidth) {
    float windowWidth = ImGui::GetWindowSize().x;
    float cursorX     = (windowWidth - itemWidth) * 0.5f;
    ImGui::SetCursorPosX(cursorX);
}

// helper: center text in ImGui window
static void CenterText(const char* text) {
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth   = ImGui::CalcTextSize(text).x;
    float posX        = (windowWidth - textWidth) * 0.5f;
    ImGui::SetCursorPosX(posX);
    ImGui::Text("%s", text);
}

// helper: center formatted text
template<typename... Args>
static void CenterTextFormatted(const char* fmt, Args&&... args) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
    CenterText(buf);
}

// render control panel with sliders and buttons
void Simulation::renderUI() {
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(SIM_WIDTH), 0));
    ImGui::SetNextWindowSize(
        ImVec2(static_cast<float>(PANEL_WIDTH),
               static_cast<float>(WINDOW_HEIGHT)));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoMove;

    ImGui::Begin("Panel", nullptr, flags);

    ImGui::Spacing();
    CenterText("CONTROL PANEL");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // start/stop button
    if (ImGui::Button(
            settings.isRunning ? "STOP SIMULATION" : "START SIMULATION",
            ImVec2(-1, 80))) {

        settings.isRunning = !settings.isRunning;

        if (settings.isRunning) {
            environment->initialize(settings.foodCount, 30, settings.foodPerCell);
            spawnAnts();
            elapsedTime        = 0.0f;
            totalFoodCollected = 0;
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    CenterText("Yellow : Scout");
    CenterText("White  : Forager");
    ImGui::Spacing();

    float sliderWidth = PANEL_WIDTH * 1.0f;

    // parameter sliders
    CenterTextFormatted("Ant Population: %d", settings.antCount);
    CenterNextItem(sliderWidth);
    ImGui::SliderInt("##count", &settings.antCount, 10, 1000);

    ImGui::Spacing();
    ImGui::Spacing();

    CenterTextFormatted("Scout Ratio: %.2f", settings.scoutRatio);
    CenterNextItem(sliderWidth);
    ImGui::SliderFloat("##scoutRatio", &settings.scoutRatio, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::Spacing();

    int scouts = 0, foragers = 0;
    for (const auto& ant : ants) {
        if (ant->getRole() == Ant::Role::Scout) scouts++;
        else foragers++;
    }
    CenterTextFormatted("Scouts: %d | Foragers: %d", scouts, foragers);

    ImGui::Spacing();
    ImGui::Spacing();


    CenterTextFormatted("Movement Speed: %.1f", settings.moveSpeed);
    CenterNextItem(sliderWidth);
    ImGui::SliderFloat("##speed", &settings.moveSpeed, 1.0f, 50.0f);

    ImGui::Spacing();
    ImGui::Spacing();

    CenterTextFormatted("Random Chaos: %.2f", settings.randomness);
    CenterNextItem(sliderWidth);
    ImGui::SliderFloat("##rand", &settings.randomness, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::Spacing();

    CenterTextFormatted("Pheromone Evaporation: %.2f", settings.evaporationRate);
    CenterNextItem(sliderWidth);
    ImGui::SliderFloat("##evap", &settings.evaporationRate, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::Spacing();

    CenterTextFormatted("Food Sources: %d", settings.foodCount);
    CenterNextItem(sliderWidth);
    ImGui::SliderInt("##foodCount", &settings.foodCount, 10, 400);

    ImGui::Spacing();
    ImGui::Spacing();

    CenterTextFormatted("Food per Cell: %.1f", settings.foodPerCell);
    CenterNextItem(sliderWidth);
    ImGui::SliderFloat("##foodPerCell",
                       &settings.foodPerCell,
                       1.0f, 20.0f);

    ImGui::End();
}

// render simulation: grid, pheromones, and ants
void Simulation::render() {
    window.clear(sf::Color(20, 20, 20));

    // draw grid cells (home, food, obstacles)
    sf::RectangleShape cellShape(
        sf::Vector2f(cellSize - 1.0f, cellSize - 1.0f));

    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            CellType type = environment->getCell(x, y);
            if (type == CellType::EMPTY) continue;

            cellShape.setPosition(x * cellSize, y * cellSize);

            if (type == CellType::HOME) {
                cellShape.setFillColor(sf::Color(50, 150, 255));
            } else if (type == CellType::FOOD) {
                cellShape.setFillColor(sf::Color(0, 200, 80));
            } else if (type == CellType::OBSTACLE) {
                cellShape.setFillColor(sf::Color(90, 60, 45));
            }
            window.draw(cellShape);
        }
    }

    // draw pheromone trails (purple, intensity based on concentration)
    sf::RectangleShape pherShape(
        sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));

    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            float pher = environment->getPheromone(x, y);
            if (pher <= 0.001f) continue;

            float intensity = std::min(1.0f, pher);
            sf::Uint8 alpha = static_cast<sf::Uint8>(255 * intensity);

            pherShape.setPosition(x * cellSize + 1.0f, y * cellSize + 1.0f);
            pherShape.setFillColor(sf::Color(180, 0, 255, alpha));
            window.draw(pherShape);
        }
    }

    // draw ants as white circles
    sf::CircleShape antShape(cellSize * 0.25f);
    
    for (const auto& ant : ants) {
        if (ant->getRole() == Ant::Role::Scout) {
            antShape.setFillColor(sf::Color(255, 200, 80));
        } else {
             antShape.setFillColor(sf::Color::White);
        }
       
        antShape.setPosition(ant->getPosition().x, ant->getPosition().y);
        window.draw(antShape);
    }

    renderUI();
    ImGui::SFML::Render(window);

    window.display();
}

#include "Simulation.hpp"
#include <iostream>

// Entry point: creates and runs the ant colony simulation
int main() {
    std::cout << "Starting Ant Colony Simulation (GUI Mode)...\n";
    std::cout << "Initializing ImGui Control Panel...\n";

    try {
        Simulation sim(80, 60); 
        sim.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Simulation finished.\n";

    std::cout << "Press ENTER to exit .\n";
    std::cin.get();

    return 0;
}

cmake_minimum_required(VERSION 3.14)

project(AntColonySimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(SFML_BUILD_AUDIO OFF CACHE INTERNAL "")
set(IMGUI_SFML_FIND_SFML OFF CACHE INTERNAL "")

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4-docking
)

FetchContent_Declare(
    imgui_sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.6
)

FetchContent_MakeAvailable(sfml imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_MakeAvailable(imgui_sfml)

file(GLOB SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(AntColonySimulation
    ${SOURCES}
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_include_directories(AntColonySimulation PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${imgui_SOURCE_DIR}"
)

target_link_libraries(AntColonySimulation PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML
)

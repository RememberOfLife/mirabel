cmake_minimum_required(VERSION 3.18)
project(mirabel)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(OpenGL REQUIRED)

find_package(SDL2 REQUIRED)

set(INCLUDES
    .
    src/
    
    lib/imgui
    lib/imgui/backends
    lib/surena
    lib/surena/games
    lib/surena/util
)

set(SOURCES
    src/main.cpp

    src/frontends/empty_frontend.cpp
    src/frontends/tictactoe.cpp

    src/games/game_catalogue.cpp
    src/games/tictactoe.cpp

    src/meta_gui/engine.cpp
    src/meta_gui/gamestate_config.cpp
    src/meta_gui/guistate_config.cpp
    src/meta_gui/logs.cpp
    src/meta_gui/main_menu.cpp
    src/meta_gui/meta_gui.cpp
    src/meta_gui/stats.cpp

    src/prototype_util/direct_draw.cpp
    src/prototype_util/st_gui.cpp

    src/state_control/controller.cpp
    src/state_control/enginethread.cpp
    src/state_control/event_queue.cpp
    src/state_control/event.cpp
    src/state_control/guithread.cpp
    
    lib/imgui/imgui.cpp 
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_sdl.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp

    lib/surena/games/surena_tictactoe.cpp
    lib/surena/util/surena_fast_prng.cpp
)

add_executable(mirabel ${SOURCES})

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(mirabel ${OPENGL_LIBRARIES})

target_link_libraries(mirabel dl)
include_directories(mirabel ${SDL2_INCLUDE_DIRS})
target_link_libraries(mirabel ${SDL2_LIBRARIES})

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

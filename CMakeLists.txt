cmake_minimum_required(VERSION 3.18)
project(mirabel)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenGL REQUIRED)

find_package(SDL2 REQUIRED)

set(INCLUDES
    lib/nanovg/src

    lib/imgui/backends
    lib/imgui

    lib/surena/includes

    src
)

set(SOURCES
    lib/nanovg/src/nanovg_gl.c
    lib/nanovg/src/nanovg.c

    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/backends/imgui_impl_sdl.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui.cpp 

    lib/surena/src/engines/randomengine.cpp
    lib/surena/src/engines/singlethreadedmcts.cpp
    lib/surena/src/games/chess.cpp
    lib/surena/src/games/havannah.cpp
    lib/surena/src/games/tictactoe_ultimate.cpp
    lib/surena/src/games/tictactoe.cpp
    lib/surena/src/util/fast_prng.cpp

    src/engines/builtin_surena.cpp
    src/engines/engine_catalogue.cpp

    src/frontends/chess.cpp
    src/frontends/empty_frontend.cpp
    src/frontends/frontend_catalogue.cpp
    src/frontends/havannah.cpp
    src/frontends/tictactoe_ultimate.cpp
    src/frontends/tictactoe.cpp

    src/games/chess.cpp
    src/games/game_catalogue.cpp
    src/games/havannah.cpp
    src/games/tictactoe_ultimate.cpp
    src/games/tictactoe.cpp

    src/meta_gui/confirm_exit.cpp
    src/meta_gui/engine.cpp
    src/meta_gui/frontend_config.cpp
    src/meta_gui/game_config.cpp
    src/meta_gui/logs.cpp
    src/meta_gui/main_menu.cpp
    src/meta_gui/meta_gui.cpp
    src/meta_gui/stats.cpp

    src/prototype_util/direct_draw.cpp
    src/prototype_util/st_gui.cpp

    src/state_control/controller.cpp
    src/state_control/event_queue.cpp
    src/state_control/event.cpp
    src/state_control/guithread.cpp
    src/state_control/timeout_crash.cpp
    
    src/main.cpp
)

add_executable(mirabel ${SOURCES})
target_compile_definitions(mirabel PUBLIC NANOVG_GL3)

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_link_libraries(mirabel Threads::Threads)

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(mirabel ${OPENGL_LIBRARIES})

target_link_libraries(mirabel dl)
include_directories(mirabel ${SDL2_INCLUDE_DIRS})
target_link_libraries(mirabel ${SDL2_LIBRARIES})

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

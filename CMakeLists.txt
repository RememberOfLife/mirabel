cmake_minimum_required(VERSION 3.18)
set (CMAKE_CXX_STANDARD 11)

project(mirabel C CXX)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(NOT CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg].*")
    if(supported)
        message(STATUS "IPO/LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: <${error}>")
    endif()
else()
    message(STATUS "IPO/LTO disabled in non-release build")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenGL REQUIRED)

find_package(SDL2 REQUIRED)

find_package(OpenSSL REQUIRED)

set(SOURCES_NANOVG
    lib/nanovg/src/nanovg_gl.c
    lib/nanovg/src/nanovg.c
)

set(INCLUDES
    lib/SDL_net

    lib/nanovg/src

    lib/imgui/backends
    lib/imgui

    lib/surena/includes

    src
)

set(SOURCES
    lib/SDL_net/SDLnet.c
    lib/SDL_net/SDLnetselect.c
    lib/SDL_net/SDLnetTCP.c
    lib/SDL_net/SDLnetUDP.c

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

    src/control/client.cpp
    src/control/event_queue.cpp
    src/control/event.cpp
    src/control/lobby.cpp
    src/control/server.cpp
    src/control/timeout_crash.cpp

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

    src/meta_gui/chat.cpp
    src/meta_gui/confirm_exit.cpp
    src/meta_gui/connection.cpp
    src/meta_gui/engine.cpp
    src/meta_gui/frontend_config.cpp
    src/meta_gui/game_config.cpp
    src/meta_gui/logs.cpp
    src/meta_gui/main_menu.cpp
    src/meta_gui/meta_gui.cpp
    src/meta_gui/stats.cpp

    src/network/network_client.cpp
    src/network/network_server.cpp

    src/prototype_util/direct_draw.cpp
    src/prototype_util/st_gui.cpp
    
    src/main.cpp
)

add_executable(mirabel ${SOURCES})

add_library(nanovg ${SOURCES_NANOVG})
target_compile_options(nanovg PRIVATE "-Wno-implicit-function-declaration")
target_compile_definitions(nanovg PUBLIC NANOVG_GL3)
target_link_libraries(mirabel nanovg)

# target_compile_options(mirabel PRIVATE "-Wall") # do after nanovg so we dont reenable warnings there

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_link_libraries(mirabel Threads::Threads)

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(mirabel ${OPENGL_LIBRARIES})

target_link_libraries(mirabel dl)
include_directories(mirabel ${SDL2_INCLUDE_DIRS})
target_link_libraries(mirabel ${SDL2_LIBRARIES})

target_link_libraries(mirabel OpenSSL::SSL)

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

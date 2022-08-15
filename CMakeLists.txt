cmake_minimum_required(VERSION 3.18)
set (CMAKE_C_STANDARD 11)
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

set(INCLUDES_NANOVG
    lib
)

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

    includes
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
    # lib/surena/src/engines/singlethreadedmcts.cpp
    lib/surena/src/games/chess.cpp
    lib/surena/src/games/havannah.cpp
    lib/surena/src/games/tictactoe_ultimate.cpp
    lib/surena/src/games/tictactoe.cpp
    lib/surena/src/games/twixt_pp.cpp
    lib/surena/src/util/base64.c
    lib/surena/src/util/fast_prng.cpp
    lib/surena/src/util/noise.c
    lib/surena/src/util/raw_stream.c
    lib/surena/src/util/semver.c
    lib/surena/src/engine.cpp
    lib/surena/src/game.c
    lib/surena/src/move_history.c

    src/control/client.cpp
    src/control/config.cpp
    src/control/event_queue.cpp
    src/control/event.cpp
    src/control/lobby.cpp
    src/control/plugins.cpp
    src/control/server.cpp
    src/control/timeout_crash.cpp

    # src/engines/builtin_surena.cpp
    src/engines/engine_manager.cpp

    src/frontends/chess.cpp
    src/frontends/empty_frontend.cpp
    src/frontends/fallback_text.cpp
    src/frontends/havannah.cpp
    src/frontends/tictactoe_ultimate.cpp
    src/frontends/tictactoe.cpp
    src/frontends/twixt_pp.cpp

    src/games/chess.cpp
    src/games/havannah.cpp
    src/games/tictactoe_ultimate.cpp
    src/games/tictactoe.cpp
    src/games/twixt_pp.cpp

    src/meta_gui/chat.cpp
    src/meta_gui/config.cpp
    src/meta_gui/confirm_exit.cpp
    src/meta_gui/connection.cpp
    src/meta_gui/engine.cpp
    src/meta_gui/frontend_config.cpp
    src/meta_gui/game_config.cpp
    src/meta_gui/global_dockspace.cpp
    src/meta_gui/history.cpp
    src/meta_gui/logs.cpp
    src/meta_gui/main_menu.cpp
    src/meta_gui/meta_gui.cpp
    src/meta_gui/plugins.cpp
    src/meta_gui/stats.cpp
    src/meta_gui/timectl.cpp

    src/network/network_client.cpp
    src/network/network_server.cpp
    src/network/protocol.cpp
    src/network/util.cpp

    src/prototype_util/direct_draw.cpp
    src/prototype_util/st_gui.cpp
    
    src/main.cpp
)

#TODO target for mirabel_server and add a define SERVER for it

add_executable(mirabel ${SOURCES})

add_library(nanovg ${SOURCES_NANOVG})
target_include_directories(nanovg PRIVATE ${INCLUDES_NANOVG})
target_compile_options(nanovg PRIVATE "-Wno-implicit-function-declaration")
target_compile_definitions(nanovg PUBLIC NANOVG_GL3)
target_link_libraries(mirabel nanovg)

target_compile_options(mirabel PRIVATE
    #"-Wall" # do after nanovg so we dont reenable warnings there
    "-Werror" # warnings as errors
    "-Wfatal-errors" # stop after first error
)

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_link_libraries(mirabel Threads::Threads)

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(mirabel ${OPENGL_LIBRARIES})

if(WIN32)
    find_package(GLEW REQUIRED) # need glew for nanovg
    target_compile_definitions(nanovg PUBLIC NANOVG_USE_GLEW)
    target_link_libraries(mirabel GLEW::GLEW)
    target_link_libraries(mirabel ws2_32 iphlpapi) # for SDL_net
    target_link_libraries(mirabel -mwindows) # we're a gui
    target_compile_definitions(mirabel PRIVATE SDL_MAIN_HANDLED) # stop WinMain panic
    target_link_libraries(mirabel SDL2::SDL2-static SDL2::SDL2main) # static sdl on windows
else()
    target_link_libraries(mirabel dl)
    target_link_libraries(mirabel SDL2::SDL2)
endif()

include_directories(mirabel ${SDL2_INCLUDE_DIRS})

target_link_libraries(mirabel OpenSSL::SSL)

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

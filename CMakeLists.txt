cmake_minimum_required(VERSION 3.18)

project(mirabel C CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenGL REQUIRED)

set(SDL_STATIC OFF CACHE INTERNAL "")
set(SDL_SHARED ON CACHE INTERNAL "")
set(SDL_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/SDL)

add_subdirectory(lib/SDL_net)

find_package(OpenSSL 3 REQUIRED)

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

    lib/rosalia/includes

    includes
    src
    "${CMAKE_BINARY_DIR}/build_generated"
)

set(SOURCES
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/backends/imgui_impl_sdl2.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui.cpp 

    lib/rosalia/src/impl/config.c
    lib/rosalia/src/impl/jobs.cpp

    src/control/auth_manager.cpp
    src/control/client.cpp
    src/control/event_queue.cpp
    src/control/event.c
    src/control/lobby_manager.cpp
    src/control/lobby.cpp
    src/control/plugins.cpp
    src/control/server.cpp
    src/control/timeout_crash.cpp
    src/control/user_manager.cpp

    # src/engines/engine_manager.cpp //TODO REENABLE engine
    # src/engines/engine.cpp //TODO REENABLE (surena) engine

    src/frontends/empty_frontend.cpp
    src/frontends/fallback_text.cpp

    src/games/game.c
    src/games/move_history.c

    src/meta_gui/about.cpp
    src/meta_gui/chat.cpp
    src/meta_gui/config.cpp
    src/meta_gui/confirm_exit.cpp
    src/meta_gui/connection.cpp
    # src/meta_gui/engine.cpp //TODO REENABLE engine
    src/meta_gui/frontend_config.cpp
    src/meta_gui/game_config.cpp
    src/meta_gui/global_dockspace.cpp
    src/meta_gui/history.cpp
    src/meta_gui/lobby.cpp
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
    src/prototype_util/imgui_c_thin.cpp
    src/prototype_util/log.cpp
    src/prototype_util/st_gui.cpp

    #TODO src/repl/repl.c

    src/rosa_impl/base64.c
    src/rosa_impl/json.c
    src/rosa_impl/noise.c
    src/rosa_impl/rand.c
    src/rosa_impl/semver.c
    src/rosa_impl/serialization.c
    src/rosa_impl/timestamp.c
    src/rosa_impl/vector.c

    src/main.cpp
)

#TODO target for mirabel_server and add a define SERVER for it

add_executable(mirabel "${SOURCES}" "${CMAKE_BINARY_DIR}/build_generated/generated/git_commit_hash.h")

set_target_properties(mirabel PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(NOT CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg].*")
    if(supported)
        message(STATUS "IPO/LTO enabled")
        set_target_properties(mirabel PROPERTIES CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: <${error}>")
    endif()
else()
    message(STATUS "IPO/LTO disabled in non-release build")
endif()

# export git commit hash and dirty flag to generated file in the build/build_generated/generated dir
find_package(Git)
set(dollar "$") # next file is processed using configure_file so we need to escape dollars to use variables
configure_file(
    cmake/generate_git_commit_hash.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/generate_git_commit_hash.cmake
    ESCAPE_QUOTES
)
add_custom_target(add_generated_dir COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/build_generated/generated")
add_custom_target(generate_git_commit_hash DEPENDS add_generated_dir COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/generate_git_commit_hash.cmake" BYPRODUCTS "${CMAKE_BINARY_DIR}/build_generated/generated/git_commit_hash.h")
add_dependencies(mirabel generate_git_commit_hash)

target_compile_options(mirabel PRIVATE
    # "-Wall"
    # "-Wextra"
    # "-Werror" # warnings as errors
    # "-Wfatal-errors" # stop after first error
    #TODO pedantic and pedantic errors
)

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_link_libraries(mirabel Threads::Threads)

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(mirabel ${OPENGL_LIBRARIES})

# prefer to use the lib sys release if it exists
file(GLOB GLEW_LIB_SYS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib-sys/*glew*/" "${CMAKE_CURRENT_SOURCE_DIR}/lib-sys/*GLEW*/")
if("${GLEW_LIB_SYS_PATH}" STREQUAL "")
    find_package(GLEW REQUIRED) #TODO is this fine with all windows build combinations?
    message(STATUS "error can not find package glew")
else()
    list(PREPEND CMAKE_PREFIX_PATH "${GLEW_LIB_SYS_PATH}")
    list(PREPEND CMAKE_LIBRARY_PATH "${GLEW_LIB_SYS_PATH}/lib/Release/")
    find_path(GLEW_INCLUDE_DIRS GL/glew.h REQUIRED)
    find_library(GLEW_LIBRARIES NAMES GLEW glew32 glew glew32s PATH_SUFFIXES x64 Win32 x86 x86_64 REQUIRED)
endif()

add_library(nanovg STATIC ${SOURCES_NANOVG})
target_include_directories(nanovg PRIVATE ${INCLUDES_NANOVG})
# target_compile_options(nanovg PRIVATE "-Wno-implicit-function-declaration")
target_compile_definitions(nanovg PUBLIC NANOVG_GL3)
target_include_directories(nanovg PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(nanovg ${OPENGL_LIBRARIES})
target_link_libraries(nanovg ${GLEW_LIBRARIES})
target_link_libraries(mirabel nanovg)
target_compile_definitions(nanovg PUBLIC NANOVG_USE_GLEW)

target_include_directories(mirabel PRIVATE ${GLEW_INCLUDE_DIRS})
target_include_directories(nanovg PRIVATE ${GLEW_INCLUDE_DIRS})
target_link_libraries(mirabel ${GLEW_LIBRARIES})

if(WIN32 AND (NOT (MINGW OR MSYS)))
    # MSVC only
    target_link_libraries(mirabel ws2_32 iphlpapi) # for SDL_net
    target_compile_definitions(mirabel PRIVATE SDL_MAIN_HANDLED) # stop WinMain panic
else()
    # linux/mingw
    target_link_libraries(mirabel dl)
    target_link_options(mirabel PRIVATE -rdynamic)
endif()

if(MINGW OR MSYS)
    target_link_libraries(mirabel -mwindows) # we're a gui
endif()

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

target_link_libraries(mirabel SDL2)
target_link_libraries(mirabel SDL2_net)

target_include_directories(mirabel PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(mirabel OpenSSL::SSL)

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

install(TARGETS mirabel RUNTIME DESTINATION .) #TODO only works for release build now

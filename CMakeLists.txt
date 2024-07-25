cmake_minimum_required(VERSION 3.18)

project(mirabel C CXX)

find_program(mold "mold")
if(NOT DEFINED EMSCRIPTEN AND NOT mold STREQUAL mold-NOTFOUND)
    set(CMAKE_LINKER_TYPE MOLD)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenGL REQUIRED)

if(NOT DEFINED EMSCRIPTEN)
    set(SDL_STATIC OFF CACHE INTERNAL "")
    set(SDL_SHARED ON CACHE INTERNAL "")
    set(SDL_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

    add_subdirectory(lib/SDL_net)

    find_package(OpenSSL 3 REQUIRED)
endif()

set(INCLUDES_NANOVG
    lib
)

set(SOURCES_NANOVG
    lib/nanovg/src/nanovg_gl.c
    lib/nanovg/src/nanovg.c
)

set(INCLUDES
    lib/crossline

    lib/imgui/backends
    lib/imgui

    lib/nanovg/src

    lib/rosalia/include

    lib/SDL_net

    include
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

    src/rosa_impl/argparse.c
    src/rosa_impl/base64.c
    src/rosa_impl/json.c
    src/rosa_impl/noise.c
    src/rosa_impl/rand.c
    src/rosa_impl/semver.c
    src/rosa_impl/serialization.c
    src/rosa_impl/timestamp.c
    src/rosa_impl/vector.c

    src/client/client.c
    src/client/workspace.c

    src/control/application.c
    src/control/event.c
    src/control/event_queue.cpp
    src/control/methods_registry.c

    src/game/game.c

    src/interface/gim/metagui/about_info.cpp
    src/interface/gim/metagui/empty_frontend.cpp
    src/interface/gim/metagui/global_dockspace.cpp
    src/interface/gim/metagui/logs.cpp
    src/interface/gim/metagui/main_menu.cpp
    src/interface/gim/metagui/workspace.cpp
    src/interface/gim/window.cpp
    src/interface/app_interface.c

    src/network/adapters/offline_client.cpp
    src/network/adapters/offline_server.cpp
    src/network/network_adapter.c
    src/network/network_connection.c

    src/server/server.c

    src/util/alloc.c
    src/util/log.cpp
)

set(SOURCES_NATIVE
    lib/crossline/crossline.c

    src/interface/crl/cli.cpp

    src/main-native.cpp
)

set(SOURCES_WEB
    src/main-web.cpp
)

#TODO separate target for mirabel_server and add a define SERVER for it, should be possible to *just* build the server, without any of the gui dependencies

if(DEFINED EMSCRIPTEN)
    add_executable(mirabel "${SOURCES}" "${SOURCES_WEB}" "${CMAKE_BINARY_DIR}/build_generated/generated/git_commit_hash.h")
else()
    add_executable(mirabel "${SOURCES}" "${SOURCES_NATIVE}" "${CMAKE_BINARY_DIR}/build_generated/generated/git_commit_hash.h")
endif()

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

target_include_directories(mirabel PRIVATE ${INCLUDES})
target_link_libraries(mirabel Threads::Threads)

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
if(NOT DEFINED EMSCRIPTEN)
    # emscripten brings its own copy, so we don't have to link it again
    target_link_libraries(mirabel ${OPENGL_LIBRARIES})
endif()

if(NOT DEFINED EMSCRIPTEN)
    # prefer to use the lib sys release if it exists
    file(GLOB GLEW_LIB_SYS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib-sys/*glew*/" "${CMAKE_CURRENT_SOURCE_DIR}/lib-sys/*GLEW*/")
    if("${GLEW_LIB_SYS_PATH}" STREQUAL "")
        find_package(GLEW REQUIRED) #TODO is this fine with all windows build combinations?
    else()
        list(PREPEND CMAKE_PREFIX_PATH "${GLEW_LIB_SYS_PATH}")
        list(PREPEND CMAKE_LIBRARY_PATH "${GLEW_LIB_SYS_PATH}/lib/Release/")
        find_path(GLEW_INCLUDE_DIRS GL/glew.h REQUIRED)
        find_library(GLEW_LIBRARIES NAMES GLEW glew32 glew glew32s PATH_SUFFIXES x64 Win32 x86 x86_64 REQUIRED)
    endif()
endif()

add_library(nanovg STATIC ${SOURCES_NANOVG})
target_include_directories(nanovg PRIVATE ${INCLUDES_NANOVG})
# target_compile_options(nanovg PRIVATE "-Wno-implicit-function-declaration")
if(DEFINED EMSCRIPTEN)
    target_compile_definitions(nanovg PUBLIC NANOVG_GLES2)
else()
    target_compile_definitions(nanovg PUBLIC NANOVG_GL3)
endif()
target_include_directories(nanovg PUBLIC ${OPENGL_INCLUDE_DIR})
# target_link_libraries(nanovg ${OPENGL_LIBRARIES}) //REMOVE if we really dont need these two but it should be fine since mirabel links these
# target_link_libraries(nanovg ${GLEW_LIBRARIES}) //REMOVE
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
    # linux/mingw/wasm
    target_link_libraries(mirabel dl)
    target_link_options(mirabel PRIVATE -rdynamic)

    target_compile_options(mirabel PRIVATE
        "-Wall"
        "-Wextra"
        # "-Werror" # warnings as errors
        "-Wfatal-errors" # stop after first error
        # "-pedantic"
        # "-pedantic-errors"
    )
endif()

if(MINGW OR MSYS)
    target_link_libraries(mirabel -mwindows) # we're a gui
endif()

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

target_include_directories(mirabel PRIVATE ${SDL2_INCLUDE_DIRS})

if(NOT DEFINED EMSCRIPTEN)
    target_link_libraries(mirabel SDL2_net)

    target_link_libraries(mirabel OpenSSL::SSL)
endif()

if(DEFINED EMSCRIPTEN)
    # -s USE_PTHREADS=1
    # --embed-file /home/urs/projects/mirabel-rv2/plugins/example/build-web/example_client.wasm@/example_client.wasm
    set_target_properties(nanovg PROPERTIES COMPILE_FLAGS "-O3 -fPIC -pthread")
    set_target_properties(mirabel PROPERTIES COMPILE_FLAGS "-O2 -s USE_SDL=2 -fPIC -pthread")
    set_target_properties(mirabel PROPERTIES LINK_FLAGS "-O2 --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/mirabel.html -s USE_SDL=2 -lwebsocket.js -s MAIN_MODULE=1 -pthread")
    set_target_properties(mirabel PROPERTIES SUFFIX ".html")
else()
    target_link_libraries(mirabel SDL2::SDL2)
    # target_link_options(SDL2 PUBLIC "-fuse-ld=mold")
    # target_link_options(nanovg PUBLIC "-fuse-ld=mold")
    # target_link_options(mirabel PUBLIC "-fuse-ld=mold")
endif()

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

#TODO enable later, for windows? and only for non web
# install(TARGETS mirabel RUNTIME DESTINATION .) #TODO only works for release build now

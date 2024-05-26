cmake_minimum_required(VERSION 3.18)

project(mirabel C CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenGL REQUIRED)

if(NOT DEFINED EMSCRIPTEN)
    set(SDL_STATIC OFF CACHE INTERNAL "")
    set(SDL_SHARED ON CACHE INTERNAL "")
    set(SDL_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
endif()

set(INCLUDES_NANOVG
    lib
)

set(SOURCES_NANOVG
    lib/nanovg/src/nanovg_gl.c
    lib/nanovg/src/nanovg.c
)

set(INCLUDES
    lib/imgui/backends
    lib/imgui

    lib/nanovg/src

    include
    src
)

set(SOURCES
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/backends/imgui_impl_sdl2.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui.cpp

    src/main.cpp
)

add_executable(mirabel "${SOURCES}")

set_target_properties(mirabel PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_link_libraries(mirabel Threads::Threads)

target_include_directories(mirabel PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(mirabel ${OPENGL_LIBRARIES})

if(NOT DEFINED EMSCRIPTEN)
    find_package(GLEW REQUIRED) # more complex find glew in reality
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
target_link_libraries(nanovg ${OPENGL_LIBRARIES})
target_link_libraries(nanovg ${GLEW_LIBRARIES})
target_link_libraries(mirabel nanovg)
target_compile_definitions(nanovg PUBLIC NANOVG_USE_GLEW)

target_include_directories(mirabel PRIVATE ${GLEW_INCLUDE_DIRS})
target_include_directories(nanovg PRIVATE ${GLEW_INCLUDE_DIRS})
target_link_libraries(mirabel ${GLEW_LIBRARIES})

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

target_include_directories(mirabel PRIVATE ${SDL2_INCLUDE_DIRS})

if(DEFINED EMSCRIPTEN)
    # -s USE_PTHREADS=1
    # --embed-file /home/urs/projects/mirabel-rv2/plugins/example/build-web/example_client.wasm@/example_client.wasm
    set_target_properties(nanovg PROPERTIES COMPILE_FLAGS "-O3 -fPIC")
    set_target_properties(mirabel PROPERTIES COMPILE_FLAGS "-O2 -s USE_SDL=2 -fPIC")
    set_target_properties(mirabel PROPERTIES LINK_FLAGS "-O2 -s USE_SDL=2 -lwebsocket.js -s MAIN_MODULE=1")
else()
    target_link_libraries(mirabel SDL2::SDL2)
endif()

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

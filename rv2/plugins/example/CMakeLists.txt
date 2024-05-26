cmake_minimum_required(VERSION 3.18)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

project(example C CXX)

find_package(OpenGL REQUIRED)

find_package(SDL2 REQUIRED)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(mirabel REQUIRED) # use -DMIRABEL_ROOT=../.. if building from within plugins directory
# version check for mirabel apis
add_library(api_version_check OBJECT "src/static_version.cpp")
target_include_directories(api_version_check PRIVATE ${MIRABEL_INCLUDE_DIRS})

set(INCLUDES
    src
)

set(SOURCES_CLIENT
    src/plugin.cpp
)

# build client
if(DEFINED EMSCRIPTEN)
    add_executable(example_client "${SOURCES_CLIENT}") # need to build to executable so we get a proper wasm side module that we can link dynamically
else()
    add_library(example_client SHARED "${SOURCES_CLIENT}")
endif()
set_target_properties(example_client PROPERTIES PREFIX "")
# set_target_properties(example_client PROPERTIES LIBRARY_OUTPUT_NAME example_client) # dont need this since we renamed it anyway
target_compile_options(example_client PRIVATE
    # "-Wall"
    # "-Wextra"
    # "-Werror" # warnings as errors
    "-Wfatal-errors" # stop after first error
)
target_include_directories(example_client PRIVATE ${INCLUDES})
target_include_directories(example_client PUBLIC ${MIRABEL_INCLUDE_DIRS})
target_link_options(example_client PRIVATE -rdynamic)
target_include_directories(example_client PUBLIC ${OPENGL_INCLUDE_DIR})
include_directories(example_client ${SDL2_INCLUDE_DIRS})

if(DEFINED EMSCRIPTEN)
    # -s USE_PTHREADS=1
    # need sdl here to compile if we use it -s USE_SDL=2
    set_target_properties(example_client PROPERTIES COMPILE_FLAGS "-O2 -fPIC")
    set_target_properties(example_client PROPERTIES LINK_FLAGS "-O2 -s SIDE_MODULE -s EXPORT_ALL=1") # replace export all by keepalive
    set_target_properties(example_client PROPERTIES SUFFIX ".wasm")
endif()

set_target_properties(example_client PROPERTIES EXPORT_COMPILE_COMMANDS true)

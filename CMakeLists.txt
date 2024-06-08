cmake_minimum_required(VERSION 3.18)

project(mirabel C CXX)

set(INCLUDES
    include
    src
)

set(SOURCES
    src/interface/crl/cli.cpp
    src/interface/gim/window.cpp

    src/main.cpp
)

set(SOURCES_NATIVE
    src/main-native.cpp
)

set(SOURCES_WEB
    src/main-web.cpp
)

if(DEFINED EMSCRIPTEN)
    add_executable(mirabel "${SOURCES}" "${SOURCES_WEB}")
else()
    add_executable(mirabel "${SOURCES}" "${SOURCES_NATIVE}")
endif()

set_target_properties(mirabel PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 11
)

target_include_directories(mirabel PRIVATE ${INCLUDES})

target_compile_options(mirabel PRIVATE
    "-Wall"
    "-Wextra"
    # "-Werror" # warnings as errors
    "-Wfatal-errors" # stop after first error
    # "-pedantic"
    # "-pedantic-errors"
)

if(DEFINED EMSCRIPTEN)
    set_target_properties(mirabel PROPERTIES COMPILE_FLAGS "-O2")
    set_target_properties(mirabel PROPERTIES LINK_FLAGS "-O2")
endif()

set_target_properties(mirabel PROPERTIES EXPORT_COMPILE_COMMANDS true)

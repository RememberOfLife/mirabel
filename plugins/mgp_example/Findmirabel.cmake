# FindMirabel.cmake

# provides the mirabel::mirabel imported target.

# if MIRABEL_ROOT is provided, use it, else search
if(NOT DEFINED MIRABEL_ROOT)
    #TODO proper search for existing mirabel installation, once mirabel installs its includes etc..
    # for now, just error out
    message(FATAL_ERROR "MIRABEL_ROOT not provided; auto-detection not yet implemented")
endif()

# extract mirabel client version
file(READ "${MIRABEL_ROOT}/src/control/client.cpp" CLIENT_CPP_CONTENTS)
string(REGEX MATCH "const semver client_version = semver{([0-9]+), ([0-9]+), ([0-9]+)};" MATCHED_STR "${CLIENT_CPP_CONTENTS}")
if(MATCHED_STR)
    set(MIRABEL_MAJOR_VERSION ${CMAKE_MATCH_1})
    set(MIRABEL_MINOR_VERSION ${CMAKE_MATCH_2})
    set(MIRABEL_PATCH_VERSION ${CMAKE_MATCH_3})
    set(MIRABEL_VERSION "${MIRABEL_MAJOR_VERSION}.${MIRABEL_MINOR_VERSION}.${MIRABEL_PATCH_VERSION}")
else()
    message(FATAL_ERROR "failed to extract version from mirabel/src/control/client.cpp")
endif()
unset(CLIENT_CPP_CONTENTS)

# include directories
set(MIRABEL_INCLUDE_DIRS
    "${MIRABEL_ROOT}/lib/nanovg/src"
    "${MIRABEL_ROOT}/lib/imgui"
    "${MIRABEL_ROOT}/lib/rosalia/includes"
    "${MIRABEL_ROOT}/includes"
)

# no libraries right now
set(MIRABEL_LIBRARIES "")

# define imported target
add_library(mirabel::mirabel INTERFACE IMPORTED)
target_include_directories(mirabel::mirabel INTERFACE "${MIRABEL_INCLUDE_DIRS}")
target_link_libraries(mirabel::mirabel INTERFACE "${MIRABEL_LIBRARIES}")

set(MIRABEL_FOUND TRUE)

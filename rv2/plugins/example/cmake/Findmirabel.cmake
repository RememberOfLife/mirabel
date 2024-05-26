# FindMirabel.cmake

# provides the mirabel::mirabel imported target.

# if MIRABEL_ROOT is provided, use it, else search
if(NOT DEFINED MIRABEL_ROOT)
    #TODO proper search for existing mirabel installation, once mirabel installs its includes etc..
    # for now, just error out
    message(FATAL_ERROR "MIRABEL_ROOT not provided; auto-detection not yet implemented")
endif()

# fake versioning
set(MIRABEL_MAJOR_VERSION 0)
set(MIRABEL_MINOR_VERSION 1)
set(MIRABEL_PATCH_VERSION 0)
set(MIRABEL_VERSION "${MIRABEL_MAJOR_VERSION}.${MIRABEL_MINOR_VERSION}.${MIRABEL_PATCH_VERSION}")

# include directories
set(MIRABEL_INCLUDE_DIRS
    "${MIRABEL_ROOT}/lib/nanovg/src"
    "${MIRABEL_ROOT}/lib/imgui"
    "${MIRABEL_ROOT}/include"
)
message("${MIRABEL_ROOT}/lib/nanovg/src")

# no libraries right now
set(MIRABEL_LIBRARIES "")

# define imported target
add_library(mirabel::mirabel INTERFACE IMPORTED)
target_include_directories(mirabel::mirabel INTERFACE "${MIRABEL_INCLUDE_DIRS}")
target_link_libraries(mirabel::mirabel INTERFACE "${MIRABEL_LIBRARIES}")

set(MIRABEL_FOUND TRUE)

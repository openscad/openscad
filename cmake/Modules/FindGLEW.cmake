#
# This find module falls back to PkgConfig if CMake's built-in
# find module doesn't find the package.
#
# Issues: It doesn't respect arguments like REQUIRED or minimum version requirement

list(REMOVE_ITEM CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(GLEW_USE_STATIC_LIBS ON)
set(GLEW_VERBOSE ON)
# FIXME: If the current macro was called with REQUIRED, CMake will pass that on to this invocation and
# abort here.
find_package(GLEW QUIET)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

if(NOT TARGET GLEW::glew)
  find_package(PkgConfig)
  pkg_search_module(GLEW glew)
  if(GLEW_FOUND)
    find_package_handle_standard_args(GLEW REQUIRED_VARS GLEW_INCLUDE_DIRS GLEW_LINK_LIBRARIES VERSION_VAR GLEW_VERSION)
    message(STATUS "GLEW_FOUND by pkg-config")
    add_library(GLEW::glew UNKNOWN IMPORTED)
    set_target_properties(GLEW::glew
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}")
    set_target_properties(GLEW::glew
      PROPERTIES INTERFACE_LINK_LIBRARIES "${GLEW_LINK_LIBRARIES}")
    list(GET GLEW_LINK_LIBRARIES 0 GLEW_IMPORTED_LOCATION)
    set_target_properties(GLEW::glew
      PROPERTIES IMPORTED_LOCATION "${GLEW_IMPORTED_LOCATION}")
  else()
    message(STATUS "GLEW not found by pkg-config")
  endif()
endif()

#
# This find module falls back to PkgConfig if CMake's built-in
# find module doesn't find the package.
#

list(REMOVE_ITEM CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
find_package(GLEW)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

if(NOT TARGET GLEW::glew)
  find_package(PkgConfig)
  pkg_search_module(GLEW)
  if(GLEW_FOUND)
    add_library(GLEW::glew UNKNOWN IMPORTED)
    set_target_properties(GLEW::glew
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}")
    set_target_properties(GLEW::glew
      PROPERTIES INTERFACE_LINK_LIBRARIES "${GLEW_LINK_LIBRARIES}")
    list(GET GLEW_LINK_LIBRARIES 0 GLEW_IMPORTED_LOCATION)
    set_target_properties(GLEW::glew
      PROPERTIES IMPORTED_LOCATION "${GLEW_IMPORTED_LOCATION}")
  endif()
endif()

# The FindGLEW module built into CMake doesn't find the MXE-packaged GLEW library,
# probably because the filename is unexpected.

find_package(PkgConfig)
pkg_search_module(GLEW glew QUIET)
if(GLEW_FOUND)
  find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_LINK_LIBRARIES GLEW_INCLUDE_DIRS)
  add_library(GLEW::glew UNKNOWN IMPORTED)
  set_target_properties(GLEW::glew
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}")
  set_target_properties(GLEW::glew
    PROPERTIES INTERFACE_LINK_LIBRARIES "${GLEW_LINK_LIBRARIES}")
  list(GET GLEW_LINK_LIBRARIES 0 GLEW_IMPORTED_LOCATION)
  set_target_properties(GLEW::glew
    PROPERTIES IMPORTED_LOCATION "${GLEW_IMPORTED_LOCATION}")
endif()

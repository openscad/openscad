include(FindPackageHandleStandardArgs)

  unset (_MSVC_INCLUDE_HINTS)
  unset (_MSVC_LIB_HINTS)

  if (MSVC AND DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND _MSVC_INCLUDE_HINTS
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
      "${CMAKE_PREFIX_PATH}/include"
    )
    list(APPEND _MSVC_LIB_HINTS
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
      "${CMAKE_PREFIX_PATH}/lib"
    )
  endif()
  
  message(STATUS "${_MSVC_INCLUDE_HINTS}")
  message(STATUS "${_MSVC_LIB_HINTS}")


find_package(PkgConfig REQUIRED)

unset (_Lib3MF_REQUIRED_VERSIONS)
unset (_Lib3MF_PC_NAME)

if (DEFINED Lib3MF_FIND_VERSION)
  if (${Lib3MF_FIND_VERSION_MAJOR} EQUAL 2)
    set (_Lib3MF_REQUIRED_VERSIONS 2)
  elseif (${Lib3MF_FIND_VERSION_MAJOR} EQUAL 1)
    set (_Lib3MF_REQUIRED_VERSIONS 2 1)
  endif()
else()
  set (_Lib3MF_REQUIRED_VERSIONS 2 1)
endif()

foreach (_Lib3MF_VERSION_MAJOR IN LISTS _Lib3MF_REQUIRED_VERSIONS)
  # Lib3MF V1 and V2 have differences in casing of the pkg-config target,
  # which matters for case-sensitive file systems.
  if (${_Lib3MF_VERSION_MAJOR} EQUAL 2)
    set(_Lib3MF_PC_NAME lib3mf)
  elseif (${_Lib3MF_VERSION_MAJOR} EQUAL 1)
    set(_Lib3MF_PC_NAME lib3MF)
  endif()

  pkg_check_modules(PC_LIB3MF ${_Lib3MF_PC_NAME} QUIET)
  if (PC_LIB3MF_FOUND)
    # pkg-config gives us the root folder which contains bindings for multiple languages.
    # We want to locate the C++ headers.
    if ("${PC_LIB3MF_VERSION}" VERSION_GREATER_EQUAL 2)
      set(FIND_HEADER "lib3mf_implicit.hpp")
    else()
      set(FIND_HEADER "Model/COM/NMR_DLLInterfaces.h")
    endif()

    find_path(Lib3MF_INCLUDE_DIRS
        NAMES "${FIND_HEADER}"
        HINTS ${PC_LIB3MF_INCLUDE_DIRS} /usr/include ${_MSVC_INCLUDE_HINTS}
        PATH_SUFFIXES lib3mf Bindings/Cpp
    )

    find_library(Lib3MF_LIBRARY
        NAMES 3mf 3MF
        HINTS ${PC_LIB3MF_LIBRARY_DIRS} ${_MSVC_LIB_HINTS}
    )

    if (Lib3MF_LIBRARY AND Lib3MF_INCLUDE_DIRS)
      # Create legacy target
      set(Lib3MF_LIBRARIES "${Lib3MF_LIBRARY}" "${LIBZIP_LIBRARY}" "${LIBZ_LIBRARY}")
      set(Lib3MF_VERSION ${PC_LIB3MF_VERSION})
      set(Lib3MF_DEFINITIONS ${PC_LIB3MF_CFLAGS_OTHER})
      break()
    endif()
  endif()
endforeach()

find_package_handle_standard_args(Lib3MF
  FOUND_VAR Lib3MF_FOUND
  VERSION_VAR Lib3MF_VERSION
  REQUIRED_VARS
    Lib3MF_LIBRARY
    Lib3MF_INCLUDE_DIRS
)

if (Lib3MF_FOUND)
  # Create imported target
  if (NOT TARGET Lib3MF::Lib3MF)
    add_library(Lib3MF::Lib3MF UNKNOWN IMPORTED)
    set_target_properties(Lib3MF::Lib3MF PROPERTIES
      IMPORTED_LOCATION "${Lib3MF_LIBRARY}"
      INTERFACE_COMPILE_OPTIONS "${Lib3MF_DEFINITIONS}"
      INTERFACE_INCLUDE_DIRECTORIES "${Lib3MF_INCLUDE_DIRS}"
    )
  endif()
endif()


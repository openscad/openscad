include(FindPackageHandleStandardArgs)

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
    find_package_handle_standard_args(Lib3MF
      VERSION_VAR PC_LIB3MF_VERSION
      REQUIRED_VARS
        PC_LIB3MF_LIBRARIES
        PC_LIB3MF_INCLUDE_DIRS
    )
    if (Lib3MF_FOUND)

      # pkg-config gives us the root folder which contains bindings for multiple languages.
      # We want to locate the C++ headers.
      if ("${PC_LIB3MF_VERSION}" VERSION_GREATER_EQUAL 2)
        set(FIND_HEADER "lib3mf_implicit.hpp")
      else()
        set(FIND_HEADER "Model/COM/NMR_DLLInterfaces.h")
      endif()

      find_path(Lib3MF_INCLUDE_DIRS
          NAMES "${FIND_HEADER}"
          HINTS ${PC_LIB3MF_INCLUDE_DIRS}
          PATH_SUFFIXES lib3mf Bindings/Cpp
      )

      find_library(Lib3MF_LIBRARY
          NAMES 3mf 3MF
          HINTS ${PC_LIB3MF_LIBRARY_DIRS}
      )

      # Create legacy target
      set(Lib3MF_LIBRARIES "${Lib3MF_LIBRARY}" "${LIBZIP_LIBRARY}" "${LIBZ_LIBRARY}")
      set(Lib3MF_VERSION ${PC_LIB3MF_VERSION})
      set(Lib3MF_DEFINITIONS ${PC_LIB3MF_CFLAGS_OTHER})

      # Create imported target
      if (NOT TARGET Lib3MF::Lib3MF)
        add_library(Lib3MF::Lib3MF UNKNOWN IMPORTED)
        set_target_properties(Lib3MF::Lib3MF PROPERTIES
          IMPORTED_LOCATION "${Lib3MF_LIBRARY}"
          INTERFACE_COMPILE_OPTIONS "${Lib3MF_DEFINITIONS}"
          INTERFACE_INCLUDE_DIRECTORIES "${Lib3MF_INCLUDE_DIRS}"
        )
      endif()

      break()
    endif()
  endif()
endforeach()

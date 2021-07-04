if( NOT CGAL_MACROS_FILE_INCLUDED )
  set(CGAL_MACROS_FILE_INCLUDED 1 )

  include("${CGAL_MODULES_DIR}/CGAL_VersionUtils.cmake")

  # Probably unused. -- Laurent Rineau, 2011/07/21
  macro(assert _arg )
    if ( NOT ${_arg} )
      message( FATAL_ERROR "Variable ${_arg} must be defined" )
    endif()
  endmacro()

  macro( hide_variable var )
    set ( ${var} ${${var}} CACHE INTERNAL "Variable hidden from user" FORCE )
  endmacro()

  macro( cache_set var )
    set ( ${var} ${ARGN} CACHE INTERNAL "" )
    set ( ${var} ${ARGN} CACHE INTERNAL "" )
  endmacro()

  macro( typed_cache_set type doc var )
    set ( ${var} ${ARGN} CACHE ${type} ${doc} FORCE )
    set ( ${var} ${ARGN} CACHE ${type} ${doc} FORCE )
  endmacro()

  macro( cache_get var )
    set ( ${var} )
  endmacro()

  # Splits inlist in the first element (head) and the rest (tail)
  macro( list_split head tail )
    set( ${head} )
    set( ${tail} )
    set( _LS_is_head TRUE )
    foreach( _LS_item ${ARGN} )
      if ( _LS_is_head )
        set( ${head} ${_LS_item} )
        set( _LS_is_head FALSE )
      else()
        list( APPEND ${tail} ${_LS_item} )
      endif()
    endforeach()
  endmacro()

  # adds elements to an internal cached list
  macro( add_to_cached_list listname )
    cache_get ( ${listname} )
    set( _ATC_${listname}_tmp  ${${listname}} )
    if ( NOT "${ARGN}" STREQUAL "" )
      list( APPEND _ATC_${listname}_tmp ${ARGN} )
    endif()
    cache_set ( ${listname} ${_ATC_${listname}_tmp} )
  endmacro()

  # adds elements to an in-memory variable named 'listname'
  macro( add_to_memory_list listname )
    if ( NOT "${ARGN}" STREQUAL "" )
      list( APPEND ${listname} ${ARGN} )
    endif()
  endmacro()

  # adds elements to a list.
  # If the first argument after 'listname' is PERSISTENT then 'listname'
  # is a persistent internal cached variable, otherwise is a memory variable.
  macro( add_to_list listname )
    list_split( _ATL_ARGN_HEAD _ATL_ARGN_TAIL ${ARGN} )
    if ( "${_ATL_ARGN_HEAD}" STREQUAL "PERSISTENT" )
      add_to_cached_list( ${listname} ${_ATL_ARGN_TAIL} )
    else()
      add_to_memory_list( ${listname} ${ARGN} )
    endif()
  endmacro()

  # Probably unused. -- Laurent Rineau, 2011/07/21
  macro( at list idx var )
    list( LENGTH ${list} ${list}_length )
    if ( ${idx} LESS ${${list}_length} )
      list( GET ${list} ${idx} ${var} )
    else()
      set( ${var} "NOTFOUND" )
    endif()
  endmacro()

  macro( found_in_list item_list item result )
    set( ${result} "FALSE" )
    foreach( element ${${item_list}} )
      if ( "${element}" STREQUAL "${item}" )
        set( ${result} "TRUE" )
      endif()
    endforeach()
  endmacro()

  macro( uniquely_add_flags target_var )
    if ( "${ARGC}" GREATER "1"  )
      set( target_list "${${target_var}}" )
      set( source_list "${ARGN}" )
      separate_arguments( target_list )
      separate_arguments( source_list )
      foreach( flag ${source_list} )
        found_in_list( target_list ${flag} ${flag}_FOUND )
        if ( NOT ${flag}_FOUND )
          typed_cache_set( STRING "User-defined flags" ${target_var} "${${target_var}} ${flag}" )
        endif()
      endforeach()
    endif()
  endmacro()

  function( CGAL_display_compiler_version )
    set(search_dirs "")
    message("Compiler version:")
    set(version "Unknown compiler. Cannot display its version")
    if(MSVC)
      execute_process(COMMAND "${CMAKE_CXX_COMPILER}"
        RESULT_VARIABLE ok
        ERROR_VARIABLE out_version
        TIMEOUT 5)
      if(ok EQUAL 0)
        set(version "${out_version}")
      endif()
    else()
      foreach(flag "-V" "--version" "-v")
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" ${flag}
          RESULT_VARIABLE ok
          OUTPUT_VARIABLE out_version
          ERROR_VARIABLE out_version
          TIMEOUT 5)
        if(ok EQUAL 0)
          if("${out_version}" MATCHES "^clang")
            execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-search-dirs
              RESULT_VARIABLE ok
              OUTPUT_VARIABLE out_search_dirs
              TIMEOUT 5)
            if(ok EQUAL 0)
              set(search_dirs "${out_search_dirs}")
            endif()
          endif()
          set(version "${out_version}")
          break()
        endif()
      endforeach()
    endif()
    message("${version}")
    if(search_dirs)
      message("Search dirs:")
      message("${search_dirs}")
    endif()
  endfunction()

  macro( get_dependency_version LIB )

    if ( "${ARGC}" GREATER "1" )
      set( PKG ${ARGV1} )
    else()
      set( PKG ${LIB} )
    endif()

    if ( ${PKG}_FOUND )

      set ( ${LIB}_VERSION "unknown" )
      if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
        #Emscripten needs tests run with nodejs.
        MESSAGE(STATUS "Emscripten detected")
        TRY_COMPILE(${LIB}_COMPILE_RES 
          "${CMAKE_BINARY_DIR}" 
          "${CGAL_INSTALLATION_PACKAGE_DIR}/config/support/print_${LIB}_version.cpp"
          CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${${PKG}_INCLUDE_DIR};${${PKG}_DEPENDENCY_INCLUDE_DIR}"
                      "-DLINK_LIBRARIES:STRING=${${PKG}_LIBRARIES};${${PKG}_DEPENDENCY_LIBRARIES}"
                      "-DLINK_DIRECTORIES:STRING=${${PKG}_LIBRARIES_DIR};${${PKG}_DEPENDENCY_LIBRARIES_DIR}"
          COPY_FILE ${LIB}_EXE
        )

        EXECUTE_PROCESS(COMMAND nodejs "${LIB}_EXE"
          RESULT_VARIABLE ${LIB}_RUN_RES
          OUTPUT_VARIABLE ${LIB}_OUTPUT
        )
      else()
        try_run( ${LIB}_RUN_RES
          ${LIB}_COMPILE_RES
          "${CMAKE_BINARY_DIR}"
          "${CGAL_INSTALLATION_PACKAGE_DIR}/config/support/print_${LIB}_version.cpp"
          CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${${PKG}_INCLUDE_DIR};${${PKG}_DEPENDENCY_INCLUDE_DIR}"
                      "-DLINK_LIBRARIES:STRING=${${PKG}_LIBRARIES};${${PKG}_DEPENDENCY_LIBRARIES}"
                      "-DLINK_DIRECTORIES:STRING=${${PKG}_LIBRARIES_DIR};${${PKG}_DEPENDENCY_LIBRARIES_DIR}"
          OUTPUT_VARIABLE ${LIB}_OUTPUT
        )
      endif(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")

      if ( ${LIB}_COMPILE_RES )

        if ( ${LIB}_RUN_RES EQUAL "0" )

          string( REGEX MATCH "version=.*\$" ${LIB}_VERSION_LINE ${${LIB}_OUTPUT}  )
          string( REPLACE "\n" "" ${LIB}_VERSION_LINE2 ${${LIB}_VERSION_LINE} )
          string( REPLACE "\r" "" ${LIB}_VERSION_LINE3 ${${LIB}_VERSION_LINE2} )
          string( REPLACE "version=" "" ${LIB}_VERSION ${${LIB}_VERSION_LINE3} )

        else()

          message( STATUS "WARNING: ${LIB} found but print_${LIB}_version.cpp exited with error condition: ${${LIB}_RUN_RES}" )
          message( STATUS "${PKG}_INCLUDE_DIR=${${PKG}_INCLUDE_DIR}" )
          message( STATUS "${PKG}_LIBRARIES=${${PKG}_LIBRARIES}" )
          message( STATUS "${PKG}_LIBRARIES_DIR=${${PKG}_LIBRARIES_DIR}" )
          message( STATUS "${${LIB}_OUTPUT}" )

        endif()

      else()

        message( STATUS "WARNING: ${LIB} found but could not compile print_${LIB}_version.cpp:")
        message( STATUS "${PKG}_INCLUDE_DIR=${${PKG}_INCLUDE_DIR}" )
        message( STATUS "${PKG}_LIBRARIES=${${PKG}_LIBRARIES}" )
        message( STATUS "${PKG}_LIBRARIES_DIR=${${PKG}_LIBRARIES_DIR}" )
        message( STATUS "${${LIB}_OUTPUT}" )

      endif()

      message( STATUS "USING ${LIB}_VERSION = '${${LIB}_VERSION}'" )

    endif()

  endmacro()

  macro( use_lib )

    set (lib "${ARGV0}")

    set (vlib ${CGAL_EXT_LIB_${lib}_PREFIX} )

    if ( ${vlib}_FOUND AND (NOT TARGET CGAL OR WITH_${lib}))

      if ( NOT ${vlib}_SETUP ) # avoid double usage

        if ( "${ARGC}" EQUAL "2" )

          set (usefile "${ARGV1}")

          include( ${usefile} )
          message (STATUS "Configured ${lib} from UseLIB-file: ${usefile}")

          # UseLIB-file has to set ${vlib}_SETUP to TRUE
          # TODO EBEB what about Qt4, Qt3, zlib?

        else()

          ####message( STATUS "${lib} include:     ${${vlib}_INCLUDE_DIR}" )
          include_directories ( SYSTEM ${${vlib}_INCLUDE_DIR} )

          # TODO EBEB remove definitions?
          ####message( STATUS "${lib} definitions: ${${vlib}_DEFINITIONS}" )
          add_definitions( ${${vlib}_DEFINITIONS} "-DCGAL_USE_${vlib}" )

          if ( ${vlib}_LIBRARIES )
            ####message( STATUS "${lib} libraries:   ${${vlib}_LIBRARIES}" )
            link_libraries( ${${vlib}_LIBRARIES} )
          endif()

          ####message (STATUS "Configured ${lib} in standard way")

          set( ${vlib}_SETUP TRUE )

        endif()

      endif()

      if (NOT ${vlib}_SETUP )

         message( WARNING "${vlib} has not been set up" )

      endif()

    else()

      if ( WITH_${lib} )
        message( SEND_ERROR "Try to use ${lib} that is not found")
      endif()

    endif()

  endmacro()


  macro( use_component component)

    message (STATUS "Requested component: ${component}")

    if(WITH_CGAL_${component})
      if(TARGET CGAL_${component})
        add_to_list( CGAL_LIBRARIES CGAL_${component} )
      else()
        add_to_list( CGAL_LIBRARIES ${CGAL_${component}_LIBRARY} )
      endif()
      add_to_list( CGAL_3RD_PARTY_LIBRARIES  ${CGAL_${component}_3RD_PARTY_LIBRARIES}  )

      add_to_list( CGAL_3RD_PARTY_INCLUDE_DIRS   ${CGAL_${component}_3RD_PARTY_INCLUDE_DIRS}   )
      add_to_list( CGAL_3RD_PARTY_DEFINITIONS    ${CGAL_${component}_3RD_PARTY_DEFINITIONS}    )
      add_to_list( CGAL_3RD_PARTY_LIBRARIES_DIRS ${CGAL_${component}_3RD_PARTY_LIBRARIES_DIRS} )

      # Nothing to add for Core

      if (${component} STREQUAL "ImageIO")
        find_package( OpenGL )
        find_package( ZLIB )
      endif()

      if (${component} STREQUAL "Qt3")
        find_package( OpenGL )
        find_package( Qt3-patched )
      endif()

      if (${component} STREQUAL "Qt4")
        find_package( OpenGL )
        find_package( Qt4 )
      endif()

    else(WITH_CGAL_${component})

      # now we are talking about 3rd party libs

      if ( ${component} STREQUAL "ALL_PRECONFIGURED_LIBS" )

        if (CGAL_ALLOW_ALL_PRECONFIGURED_LIBS_COMPONENT)
          message( STATUS "External libraries are all used")
          foreach ( CGAL_3RD_PARTY_LIB ${CGAL_SUPPORTING_3RD_PARTY_LIBRARIES})
            if (${CGAL_3RD_PARTY_LIB}_FOUND)
              use_lib( ${CGAL_3RD_PARTY_LIB} ${${CGAL_3RD_PARTY_LIB}_USE_FILE})
            endif()
          endforeach()
        else()
          message( SEND_ERROR "Component ALL_PRECONFIGURED_LIBS only allow with CGAL_ALLOW_ALL_PRECONFIGURED_LIBS_COMPONENT=ON")
        endif()

      else()

        list( FIND CGAL_CONFIGURED_LIBRARIES "CGAL_${component}" POSITION )
        if ( "${POSITION}" EQUAL "-1" ) # if component is not a CGAL_<lib>

          if (NOT DEFINED CGAL_EXT_LIB_${component}_PREFIX)
            set(CGAL_EXT_LIB_${component}_PREFIX ${component})
          endif()

          set( vlib "${CGAL_EXT_LIB_${component}_PREFIX}" )

          if ( NOT CGAL_IGNORE_PRECONFIGURED_${component} AND ${vlib}_FOUND)

            ####message( STATUS "External library ${component} has been preconfigured")
            use_lib( ${component} ${${vlib}_USE_FILE})

          else()

            ####message( STATUS "External library ${component} has not been preconfigured")
            find_package( ${component} )
            ####message( STATUS "External library ${vlib} after find")
            if (${vlib}_FOUND)
              ####message( STATUS "External library ${vlib} about to be used")
              use_lib( ${component} ${${vlib}_USE_FILE})
            endif()

          endif()
        else()

          if (NOT WITH_CGAL_${component}) 
            message(STATUS "NOTICE: The CGAL_${component} library seems to be required but is not build. Thus, it is expected that some executables will not be compiled.")
          endif()

        endif()

      endif()

    endif(WITH_CGAL_${component})

  endmacro()

  macro( use_essential_libs )

    # Comment: This is subject to be changed in the future
    #          - either more specific (giving precise include_dir- and link-order)
    #          - or even less specific if order becomes less relevant
    # Eric Berberich 2012/06/29

    if(RS_FOUND)
      use_component( RS )
    endif()

    if(MPFI_FOUND)
      use_component( MPFI )
    endif()

    use_component( MPFR )
    if (GMPXX_FOUND)
      use_component( GMPXX )
    endif()
    use_component( GMP )

    if(LEDA_FOUND)
      use_component( LEDA )
    endif()

    if(NTL_FOUND)
      use_component( NTL )
    endif()
  endmacro()


  function( cgal_setup_module_path )
    # Avoid to modify the modules path twice
    if(NOT CGAL_MODULE_PATH_IS_SET)
      # Where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
      set(CGAL_CMAKE_MODULE_PATH ${CGAL_MODULES_DIR})

      # Use FindQt4 from CMake-2.8.1 if 2.6.2 <= CMake <= 2.8.1
      if(CMAKE_VERSION)
        is_version_less("2.6.1" "${CMAKE_VERSION}" CMAKE_VERSION_BETWEEN_2_6_2_AND_2_8_1)
      else()
        # It seems CMake <= 2.6.1 does not has that CMAKE_VERSION
        set(CMAKE_VERSION_BETWEEN_2_6_2_AND_2_8_1 FALSE)
      endif()
      if(CMAKE_VERSION_BETWEEN_2_6_2_AND_2_8_1)
        is_version_less(${CMAKE_VERSION} "2.8.2" CMAKE_VERSION_BETWEEN_2_6_2_AND_2_8_1)
        if(CMAKE_VERSION_BETWEEN_2_6_2_AND_2_8_1)
          set(CGAL_CMAKE_MODULE_PATH ${CGAL_CMAKE_MODULE_PATH} ${CGAL_MODULES_DIR}/2.6.2-to-2.8.1)
        endif()
      endif()

      set(ORIGINAL_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)

      set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CGAL_CMAKE_MODULE_PATH})

      # Export those variables to the parent scope (the scope that calls the function)
      set(CGAL_MODULE_PATH_IS_SET TRUE PARENT_SCOPE)
      set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
      set(CGAL_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
      set(ORIGINAL_CMAKE_MODULE_PATH "${ORIGINAL_CMAKE_MODULE_PATH}" PARENT_SCOPE)
    endif()
  endfunction()

  macro( create_CGALconfig_files )

    # CGALConfig.cmake is platform specific so it is generated and stored in the binary folder.
    configure_file("${CGAL_MODULES_DIR}/CGALConfig_binary.cmake.in"  "${CMAKE_BINARY_DIR}/CGALConfig.cmake"        @ONLY)

    # There is also a version of CGALConfig.cmake that is prepared in case CGAL in installed in CMAKE_INSTALL_PREFIX.
    configure_file("${CGAL_MODULES_DIR}/CGALConfig_install.cmake.in" "${CMAKE_BINARY_DIR}/config/CGALConfig.cmake" @ONLY)

    #write prefix exceptions
    file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "${SPECIAL_PREFIXES}\n")
    file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "${SPECIAL_PREFIXES}")

     foreach( lib ${CGAL_SUPPORTING_3RD_PARTY_LIBRARIES} )

       list( FIND CGAL_ESSENTIAL_3RD_PARTY_LIBRARIES "${lib}" POSITION )
       # if lib is essential or preconfiguration for an activated library ...
       if ( ("${POSITION}" STRGREATER "-1") OR ( CGAL_ENABLE_PRECONFIG AND WITH_${lib} ))

         set (vlib ${CGAL_EXT_LIB_${lib}_PREFIX} )
         #the next 'if' is needed to avoid ${vlib} config variables to be overidden in case of a local configuration change
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "if (NOT CGAL_IGNORE_PRECONFIGURED_${lib})\n")
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "  set( ${vlib}_FOUND           \"${${vlib}_FOUND}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "  set( ${vlib}_USE_FILE        \"${${vlib}_USE_FILE}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "  set( ${vlib}_INCLUDE_DIR     \"${${vlib}_INCLUDE_DIR}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "  set( ${vlib}_LIBRARIES       \"${${vlib}_LIBRARIES}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "  set( ${vlib}_DEFINITIONS     \"${${vlib}_DEFINITIONS}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/CGALConfig.cmake "endif()\n\n")


         #the next 'if' is needed to avoid ${vlib} config variables to be overidden in case of a local configuration change
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "if (NOT CGAL_IGNORE_PRECONFIGURED_${lib})\n")
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "  set( ${vlib}_FOUND           \"${${vlib}_FOUND}\")\n")
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "  set( ${vlib}_USE_FILE        \"${${vlib}_USE_FILE}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "  set( ${vlib}_INCLUDE_DIR     \"${${vlib}_INCLUDE_DIR}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "  set( ${vlib}_LIBRARIES       \"${${vlib}_LIBRARIES}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "  set( ${vlib}_DEFINITIONS     \"${${vlib}_DEFINITIONS}\" )\n")
         file( APPEND ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake "endif()\n\n")
       endif()

     endforeach()

  endmacro()

  macro ( fetch_env_var VAR )
    if ( "${${VAR}}" STREQUAL "" )
      set( ${VAR}_env_value "$ENV{${VAR}}" )
      if ( NOT "${${VAR}_env_value}" STREQUAL "" )
        # Convert Windows path to Unix-style
        FILE(TO_CMAKE_PATH ${${VAR}_env_value} ${VAR})
      endif()
    endif()
  endmacro()


## All the following macros are probably unused. -- Laurent Rineau, 2011/07/21

  # Composes a tagged list of libraries: a list with interpersed keywords or tags
  # indicating that all following libraries, up to the next tag, are to be linked only for the
  # corresponding build type. The 'general' tag indicates libraries that corresponds to all build types.
  # 'optimized' corresponds to release builds and 'debug' to debug builds. Tags are case sensitve and
  # the inital range of libraries listed before any tag is implicitely 'general'
  #
  # This macro takes 3 lists of general, optimized and debug libraries, resp, and populates the list
  # given in the fourth argument.
  #
  # The first three parameters must be strings containing a semi-colon separated list of elements.
  # All three lists must be passed, but any of them can be an empty string "".
  # The fourth parameter, corresponding to the result, must be a variable name and it will be APPENDED
  # (retaining any previous contents)
  #
  # If there is a last parameter whose value is "PERSISTENT" then the result is an internal cached variable,
  # otherwise it is an in-memory variable
  #
  macro( compose_tagged_libraries libs_general libs_optimized libs_debug libs )

    if ( "${ARGN}" STREQUAL "PERSISTENT" )
      set( _CTL_IN_CACHE "PERSISTENT" )
    else()
      set( _CTL_IN_CACHE )
    endif()

    if ( NOT "${libs_general}" STREQUAL "" )
      add_to_list( ${libs} ${_CTL_IN_CACHE} ${libs_general} )
    endif()

    if ( NOT "${libs_optimized}" STREQUAL "" )
      add_to_list( ${libs} ${_CTL_IN_CACHE} optimized ${libs_optimized} )
    endif()

    if ( NOT "${libs_debug}" STREQUAL "" )
      add_to_list( ${libs} ${_CTL_IN_CACHE} debug ${libs_debug} )
    endif()

  endmacro()

  # Decomposes a tagged list of libraries (see macro compose_tagged_libraries).
  # The first argument is the tagged list and the next 3 arguments are the lists
  # where the general, optimized and debug libraries are collected.
  #
  # The first parameter must be a string containing a semi-colon separated list of elements.
  # It cannot be ommitted, but it can be an empty string ""
  #
  # TThe next three arguments must be the names of the variables containing the result, and they
  # will be APPENDED (retaining any previous contents)
  #
  # If there is a last parameter whose value is "PERSISTENT" then the result variables are internal in the cache,
  # otherwise they are in-memory.
  #
  macro( decompose_tagged_libraries libs libs_general libs_optimized libs_debug )

    if ( "${ARGN}" STREQUAL "PERSISTENT" )
      set( _DTL_IN_CACHE "PERSISTENT" )
    else()
      set( _DTL_IN_CACHE )
    endif()

    set( _DTL_tag general )

    foreach( _DTL_lib ${libs} )

      if ( "${_DTL_lib}" STREQUAL "general" OR "${_DTL_lib}" STREQUAL "optimized" OR "${_DTL_lib}" STREQUAL "debug" )

        set( _DTL_tag "${_DTL_lib}" )

      else()

        if (     "${_DTL_tag}" STREQUAL "general"   )
                                                       set( _DTL_target ${libs_general}   )
        elseif ( "${_DTL_tag}" STREQUAL "optimized" )
                                                       set( _DTL_target ${libs_optimized} )
        else()
                                                       set( _DTL_target ${libs_debug}     )
        endif()

        add_to_list( ${_DTL_target} ${_DTL_IN_CACHE} ${_DTL_lib} )

      endif()

    endforeach()

  endmacro()

  # Given lists of optimized and debug libraries, creates a tagged list which will
  # contain the libraries listed in the 'general' section if any of the two lists is empty,
  #
  # All arguments are variable names (not values), thus the input list can be undefined or empty.
  # The return variable ('libs') will be APPENDED the result (retaining any previous contents)
  #
  # If there is a last parameter whose value is "PERSISTENT" then the result is an internal cached variable,
  # otherwise it is an in-memory variable
  #
  # Example:
  #
  #   set( LIBS_1 libA.so libB.so )
  #   set( LIBS_2 libC.so )
  #
  #   tag_libraries( LIBS_1 LIBS_2 LIBS_R )
  #
  # LIBS_R -> optimized;libA.so;libB.so;debug;libC.so
  #
  #   tag_libraries( LIBS_1 SOME_UNDEFINED_VARIABLE_OR_EMPTY_LIST LIBS_R )
  #
  # LIBS_R -> libA.so;libB.so  (implicitely 'general' since there is no tag)
  #
  #   tag_libraries( SOME_UNDEFINED_VARIABLE_OR_EMPTY_LIST LIBS_2 LIBS_R )
  #
  # LIBS_R -> libC.so  (implicitely 'general' since there is no tag)
  #
  macro( tag_libraries libs_general_or_optimized libs_general_or_debug libs )

    list( LENGTH ${libs_general_or_optimized} _TL_libs_general_or_optimized_len )
    list( LENGTH ${libs_general_or_debug}     _TL_libs_general_or_debug_len     )

    if ( _TL_libs_general_or_optimized_len EQUAL 0 )
                                                     compose_tagged_libraries( "${${libs_general_or_debug}}"     ""                                 ""                           ${libs} ${ARGN} )
    elseif ( _TL_libs_general_or_debug_len EQUAL 0 )
                                                     compose_tagged_libraries( "${${libs_general_or_optimized}}" ""                                 ""                           ${libs} ${ARGN} )
    else()
                                                     compose_tagged_libraries( ""                                "${${libs_general_or_optimized}}" "${${libs_general_or_debug}}" ${libs} ${ARGN} )
    endif()

  endmacro()

  # add_to_tagged_libraries( libsR ${libsA} <PERSISTENT> )
  #
  # Appends the list of tagged libraries contained in the variable 'libA' to the list
  # of tagged libraries contained in the variable 'libR', properly redistributing each tagged subsequence.
  #
  # The first argument is the name of the variable recieving the list. It will be APPENDED
  # (retaining any previous contents).
  # The second parameter is a single string value containing the tagged
  # lists of libraries to append (as a semi-colon separated list). It can be empty, in which case noting is added.
  #
  # If there is a third parameter whose value is PERSISTENT, then 'libR' is an internal cached variable, otherwise
  # it is an in-memory variable.
  #
  # It is not possible to append more than one list in the same call.
  #
  # Example:
  #
  #   set( LIBS_1 libG0.so libG1.so optimized libO0.so)
  #   set( LIBS_2 libG2.so debug libD0.so)
  #   set( LIBS_3 debug libD1.so optimized libO1.so libO2.so )
  #
  #   concat_tagged_libraries( LIBS_R ${LIBS_1} PERSISTENT )
  #   concat_tagged_libraries( LIBS_R ${LIBS_2} PERSISTENT )
  #   concat_tagged_libraries( LIBS_R ${LIBS_3} PERSISTENT )
  #
  # LIBS_R -> libG0.so;libG1.so;libG2.so;optimized;libO0.so;libO1.so;libO2.so;debug;libD0.so;libD1.so, in the cache
  #
  macro( add_to_tagged_libraries libsR in_cache libsA  )

    if ( "${in_cache}" STREQUAL "PERSISTENT" )
      set( _CTL_IN_CACHE "PERSISTENT" )
    else()
      set( _CTL_IN_CACHE )
    endif()

    set( _CTL_general_0   )
    set( _CTL_optimized_0 )
    set( _CTL_debug_0     )
    set( _CTL_general_1   )
    set( _CTL_optimized_1 )
    set( _CTL_debug_0     )

    decompose_tagged_libraries( "${${libsR}}" _CTL_general_0 _CTL_optimized_0 _CTL_debug_0 )
    decompose_tagged_libraries( "${libsA}"    _CTL_general_1 _CTL_optimized_1 _CTL_debug_1 )

    add_to_list( _CTL_general_0   ${_CTL_general_1}   )
    add_to_list( _CTL_optimized_0 ${_CTL_optimized_1} )
    add_to_list( _CTL_debug_0     ${_CTL_debug_1}     )

    if ( "${_CTL_IN_CACHE}" STREQUAL "PERSISTENT" )
      cache_set( ${libsR} )
    else()
      set( ${libsR} )
    endif()

    compose_tagged_libraries( "${_CTL_general_0}" "${_CTL_optimized_0}" "${_CTL_debug_0}" ${libsR} ${_CTL_IN_CACHE} )

  endmacro()


  macro( add_to_persistent_tagged_libraries libsR )
    add_to_tagged_libraries( ${libsR} PERSISTENT "${ARGN}" )
  endmacro()

  macro( add_to_in_memory_tagged_libraries libsR )
    add_to_tagged_libraries( ${libsR} NOT_PERSISTENT "${ARGN}" )
  endmacro()


endif()


function(process_CGAL_subdirectory entry subdir type_name)
  # For example, subdir can be "examples", type_name "example", and entry "Mesh_2"

  if ( CGAL_BRANCH_BUILD )
    string( REGEX REPLACE "${CMAKE_SOURCE_DIR}/.*/${subdir}/" "" ENTRY_DIR_NAME "${entry}" )
  else()
    string( REGEX REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" ENTRY_DIR_NAME "${entry}" )
  endif()

  if( NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}") # out-of-source
    make_directory("${CMAKE_BINARY_DIR}/${subdir}/${ENTRY_DIR_NAME}")
  endif()

  set(ADD_SUBDIR TRUE)

  if(EXISTS ${entry}/../../dont_submit)
    file(STRINGS ${entry}/../../dont_submit dont_submit_grep REGEX "^${ENTRY_DIR_NAME}/?\$")
    if(dont_submit_grep)
      set(ADD_SUBDIR FALSE)
    endif()
    file(STRINGS ${entry}/../../dont_submit dont_submit_grep REGEX "^${subdir}/${ENTRY_DIR_NAME}/?\$")
    if(dont_submit_grep)
      set(ADD_SUBDIR FALSE)
    endif()
    file(STRINGS ${entry}/../../dont_submit dont_submit_grep REGEX "^${subdir}/?\$")
    if(dont_submit_grep)
      set(ADD_SUBDIR FALSE)
    endif()
  endif()

  if(ADD_SUBDIR)
    message("\n-- Configuring ${subdir} in ${subdir}/${ENTRY_DIR_NAME}")
    if(EXISTS ${entry}/CMakeLists.txt)
      add_subdirectory( ${entry} ${CMAKE_BINARY_DIR}/${subdir}/${ENTRY_DIR_NAME} )
    else()
      if(CGAL_CREATE_CMAKE_SCRIPT)
#        message("bah ${CGAL_CREATE_CMAKE_SCRIPT} ${type_name} --source_dir ${entry}")
        execute_process(
          COMMAND bash ${CGAL_CREATE_CMAKE_SCRIPT} ${type_name} --source_dir "${entry}"
          WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${subdir}/${ENTRY_DIR_NAME}"
          RESULT_VARIABLE RESULT_VAR OUTPUT_QUIET)
        if(NOT RESULT_VAR)
#          message("Subdir ${CMAKE_BINARY_DIR}/${subdir}/${ENTRY_DIR_NAME}")
          add_subdirectory( "${CMAKE_BINARY_DIR}/${subdir}/${ENTRY_DIR_NAME}" "${CMAKE_BINARY_DIR}/${subdir}/${ENTRY_DIR_NAME}")
        endif()
      endif()
    endif()
  else()
    message(STATUS "${subdir}/${ENTRY_DIR_NAME} is in dont_submit")
  endif()
endfunction()


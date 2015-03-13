# Top level CMakeLists.txt for CGAL
# The name of our project is "CGAL".  CMakeLists files in this project can
# refer to the root source directory of the project as ${CMAKE_SOURCE_DIR} or
# ${CMAKE_SOURCE_DIR} and to the root binary directory of the project as
# ${CMAKE_BINARY_DIR} or ${CMAKE_BINARY_DIR}.
project(CGAL CXX C)

# Minimal version of CMake:
if(WIN32)
  cmake_minimum_required(VERSION 2.8.6)
else()
  cmake_minimum_required(VERSION 2.6.2)
endif()

# Tested version:
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.3)
  cmake_policy(VERSION 2.8.4)
else()
  cmake_policy(VERSION 2.6)
endif()

#--------------------------------------------------------------------------------------------------
#
#                                    -= PACKAGE SETUP =-
#
#--------------------------------------------------------------------------------------------------

message( "== Setting paths ==" )

if ( CGAL_BRANCH_BUILD )

  message( STATUS "Build CGAL from ${CGAL_SCM_NAME}-branch: ${CGAL_SCM_BRANCH_NAME}" )

  # list packages
  file(GLOB CGAL_CONFIGURED_PACKAGES RELATIVE ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/*")
  list(REMOVE_ITEM CGAL_CONFIGURED_PACKAGES copyright CMakeLists.txt .svn .git)

  # detect and remove not existing package-directories
  foreach (package ${CGAL_CONFIGURED_PACKAGES})
    if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${package}")
      list(APPEND CGAL_WRONG_PACKAGES ${package})
    elseif(NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${package}/package_info")
      list(APPEND CGAL_WRONG_PACKAGES ${package})
    endif()
  endforeach()

  find_program(CGAL_CREATE_CMAKE_SCRIPT cgal_create_cmake_script
               HINT ${CMAKE_SOURCE_DIR}/Scripts/scripts
	       DOC "Script cgal_create_cmake_script, that creates CMakeLists.txt files"
	       NO_DEFAULT_PATH
	       NO_CMAKE_ENVIRONMENT_PATH)

  if (DEFINED CGAL_WRONG_PACKAGES)
    message(STATUS "Removed not-a-package: ${CGAL_WRONG_PACKAGES}")
    list(REMOVE_ITEM CGAL_CONFIGURED_PACKAGES ${CGAL_WRONG_PACKAGES})
  endif()

  set(CGAL_CONFIGURED_PACKAGES_NAMES ${CGAL_CONFIGURED_PACKAGES})
  set(CGAL_CONFIGURED_PACKAGES)
  foreach (package ${CGAL_CONFIGURED_PACKAGES_NAMES})
    list(APPEND CGAL_CONFIGURED_PACKAGES "${CMAKE_SOURCE_DIR}/${package}")
  endforeach()


  set(CGAL_INSTALLATION_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/Installation" CACHE INTERNAL "Directory containing the Installation package")
  set(CGAL_MAINTENANCE_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/Maintenance" CACHE INTERNAL "Directory containing the Maintenance package")
  set(CGAL_CORE_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/Core" CACHE INTERNAL "Directory containing the Core package")

  message(STATUS "Installation package directory: ${CGAL_INSTALLATION_PACKAGE_DIR}")
  message(STATUS "Maintenance package directory: ${CGAL_MAINTENANCE_PACKAGE_DIR}")
  message(STATUS "Core package directory: ${CGAL_CORE_PACKAGE_DIR}")

else ( CGAL_BRANCH_BUILD )

  # get release name
  file(TO_CMAKE_PATH ${CMAKE_SOURCE_DIR} CMAKE_SOURCE_CDIR)
  string(REGEX REPLACE "^/" "" CMAKE_SOURCE_DDIR ${CMAKE_SOURCE_CDIR})
  string(REPLACE "/" ";" CMAKE_SOURCE_PDIR ${CMAKE_SOURCE_DDIR})
  list(GET CMAKE_SOURCE_PDIR -1 CGAL_RELEASE_NAME)
  message( STATUS "Build CGAL from release in directory ${CGAL_RELEASE_NAME}" )

  set(CGAL_CONFIGURED_PACKAGES_NAMES ${CGAL_RELEASE_NAME})

  # list packages
  set(CGAL_CONFIGURED_PACKAGES "${CMAKE_SOURCE_DIR}")

  set(CGAL_INSTALLATION_PACKAGE_DIR "${CMAKE_SOURCE_DIR}" CACHE INTERNAL "Directory containing the Installation package")
  set(CGAL_MAINTENANCE_PACKAGE_DIR "${CMAKE_SOURCE_DIR}" CACHE INTERNAL "Directory containing the Maintenance package")
  set(CGAL_CORE_PACKAGE_DIR "${CMAKE_SOURCE_DIR}" CACHE INTERNAL "Directory containing the Core package")

endif (CGAL_BRANCH_BUILD )

#message(STATUS "Packages found: ${CGAL_CONFIGURED_PACKAGES}")

list(SORT CGAL_CONFIGURED_PACKAGES_NAMES)
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/Documentation")
  # Rescope CGAL_CONFIGURED_PACKAGES_NAMES
  # We need this variable in the Doxygen configuration process. Reset it
  # and change the scope to the parent scope.
  # Philipp Moeller, 2013-05-302013-05-302013-05-30
  set(CGAL_CONFIGURED_PACKAGES_NAMES ${CGAL_CONFIGURED_PACKAGES_NAMES} PARENT_SCOPE)
endif()

message(STATUS "Packagenames: ${CGAL_CONFIGURED_PACKAGES_NAMES}")
message( "== Setting paths (DONE) ==\n" )

message( "== Generate version files ==")

if ( CGAL_BRANCH_BUILD ) 
    
  #
  # Create version files
  #

  file(STRINGS "${CGAL_MAINTENANCE_PACKAGE_DIR}/release_building/MAJOR_NUMBER" CGAL_MAJOR_VERSION REGEX "[0-9]*")
  file(STRINGS "${CGAL_MAINTENANCE_PACKAGE_DIR}/release_building/MINOR_NUMBER" CGAL_MINOR_VERSION REGEX "[0-9]*")
  file(STRINGS "${CGAL_MAINTENANCE_PACKAGE_DIR}/release_building/BUGFIX_NUMBER" CGAL_BUGFIX_VERSION REGEX "[0-9]*")

  set(CGAL_BUILD_VERSION 900)
  file(REMOVE ${CMAKE_BINARY_DIR}/VERSION)
  if (CGAL_BUGFIX_VERSION AND CGAL_BUGFIX_VERSION GREATER 0)  
    set(CGAL_CREATED_VERSION_NUM "${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}.${CGAL_BUGFIX_VERSION}")
  else()
    set(CGAL_CREATED_VERSION_NUM "${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}")
  endif()
  
  # Accessed in the documentation.
  set(CGAL_CREATED_VERSION_NUM ${CGAL_CREATED_VERSION_NUM} PARENT_SCOPE)

  
  
  # TODO EBEB set number of internal release (replace "900")
  set(CGAL_CREATED_VERSION "${CGAL_CREATED_VERSION_NUM}-I-${CGAL_BUILD_VERSION}")
  set(CGAL_VERSION "${CGAL_CREATED_VERSION_NUM}-I-${CGAL_BUILD_VERSION}")

  file(WRITE "${CMAKE_BINARY_DIR}/VERSION" "${CGAL_CREATED_VERSION_NUM}")

  message(STATUS "CGAL_VERSION is ${CGAL_CREATED_VERSION}")
  set(CGAL_FULL_VERSION ${CGAL_CREATED_VERSION})


  #define CGAL_VERSION_NR 1030500135
  string (LENGTH "${CGAL_MAJOR_VERSION}" CGAL_MAJOR_LEN)
  if ("${CGAL_MAJOR_LEN}" EQUAL 1) 
    set(CGAL_TMP_MAJOR "0${CGAL_MAJOR_VERSION}")
  endif()
  string (LENGTH "${CGAL_MINOR_VERSION}" CGAL_MINOR_LEN)
  if ("${CGAL_MINOR_LEN}" EQUAL 1) 
    set(CGAL_TMP_MINOR "0${CGAL_MINOR_VERSION}")
  else()
    set(CGAL_TMP_MINOR "${CGAL_MINOR_VERSION}")
  endif()

  set(CGAL_TMP_BUILD "${CGAL_BUILD_VERSION}")
  string(LENGTH "${CGAL_TMP_BUILD}" CGAL_BUILD_LEN)

  while(CGAL_BUILD_LEN LESS 4)
    set(CGAL_TMP_BUILD "0${CGAL_TMP_BUILD}")
    string(LENGTH "${CGAL_TMP_BUILD}" CGAL_BUILD_LEN)
  endwhile()

  set(CGAL_CREATED_VERSION_NR "1${CGAL_TMP_MAJOR}${CGAL_TMP_MINOR}${CGAL_BUGFIX_VERSION}${CGAL_TMP_BUILD}")
  message(STATUS "CGAL_VERSION_NR is ${CGAL_CREATED_VERSION_NR}")

  message(STATUS "CGAL_GIT_HASH is ${CGAL_GIT_HASH}")
  message(STATUS "CGAL_CREATED_SVN_REVISION is ${CGAL_CREATED_SVN_REVISION} (dummy)")

  file(REMOVE ${CMAKE_BINARY_DIR}/include/CGAL/version.h)
  configure_file(${CGAL_INSTALLATION_PACKAGE_DIR}/config/version.h.in ${CMAKE_BINARY_DIR}/include/CGAL/version.h @ONLY)

  #
  # Duplicate files
  #

  if (CGAL_REPORT_DUPLICATE_FILES) 

    message("== Searching for duplicate files ==")

    foreach (package ${CGAL_CONFIGURED_PACKAGES})

      file(GLOB_RECURSE CGAL_PACKAGE_HEADER_FILES FOLLOW_SYMLINKS ${package}/*.h)
      foreach (file ${CGAL_PACKAGE_HEADER_FILES})
        string(REPLACE "${package}/" "" pure_file ${file})
        if (NOT ${pure_file} MATCHES ".*CMakeFiles.*")
          list(APPEND CGAL_HEADER_FILES ${pure_file})
        endif()
      endforeach()

      file(GLOB_RECURSE CGAL_PACKAGE_CPP_FILES FOLLOW_SYMLINKS ${package}/*.cpp)
      foreach (file ${CGAL_PACKAGE_CPP_FILES})
        string(REPLACE "${package}/" "" pure_file ${file})
        if (NOT ${pure_file} MATCHES ".*CMakeFiles.*")
          list(APPEND CGAL_CPP_FILES ${pure_file})
        endif()
      endforeach()

    endforeach()

    list(SORT CGAL_HEADER_FILES)
    set(CGAL_UNIQUE_HEADER_FILES ${CGAL_HEADER_FILES})
    set(CGAL_DUPLICATE_HEADER_FILES ${CGAL_HEADER_FILES})
    list(REMOVE_DUPLICATES CGAL_UNIQUE_HEADER_FILES)
    #message(STATUS "Headers: ${CGAL_HEADER_FILES}")
    foreach (file ${CGAL_UNIQUE_HEADER_FILES})
      list(FIND CGAL_DUPLICATE_HEADER_FILES "${file}" CGAL_FILE_IDX)
      list(REMOVE_AT CGAL_DUPLICATE_HEADER_FILES ${CGAL_FILE_IDX})
    endforeach()
    message(STATUS "Duplicate header files (will be repeated at end): ${CGAL_DUPLICATE_HEADER_FILES}")

    list(SORT CGAL_CPP_FILES)
    set(CGAL_UNIQUE_CPP_FILES ${CGAL_CPP_FILES})
    set(CGAL_DUPLICATE_CPP_FILES ${CGAL_CPP_FILES})
    list(REMOVE_DUPLICATES CGAL_UNIQUE_CPP_FILES)
    #message(STATUS "CPPs: ${CGAL_CPP_FILES}")
    foreach (file ${CGAL_UNIQUE_CPP_FILES})
      list(FIND CGAL_DUPLICATE_CPP_FILES "${file}" CGAL_FILE_IDX)
      list(REMOVE_AT CGAL_DUPLICATE_CPP_FILES ${CGAL_FILE_IDX})
    endforeach()
    message(STATUS "Duplicate cpp files (will be repeated at end): ${CGAL_DUPLICATE_CPP_FILES}")

  endif(CGAL_REPORT_DUPLICATE_FILES)

else()

  if (NOT EXISTS "${CMAKE_SOURCE_DIR}/VERSION" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/include/CGAL/version.h")
    message(FATAL_ERROR "File VERSION or include/CGAL/version.h are missing, required by release-build.")
  endif()

  # 
  # Read and parse CGAL version number from VERSION file
  #

  file( READ "${CMAKE_SOURCE_DIR}/VERSION" CGAL_VERSION )
  # Comment from Laurent Rineau, 2012/06/28:
  #   file(READ ..) should be replace by file(STRINGS ..), and then we take
  #   the first line. That would avoid parsing errors when the VERSION file
  #   contains a CR character!

  set(CGAL_FULL_VERSION ${CGAL_VERSION})
  string( REPLACE "-" "." CGAL_VERSION_TOKENS1 ${CGAL_VERSION} )
  string( REPLACE "." ";" CGAL_VERSION_TOKENS ${CGAL_VERSION_TOKENS1} )
  list( LENGTH CGAL_VERSION_TOKENS CGAL_VERSION_TOKENS_LEN )
  list( GET CGAL_VERSION_TOKENS 0 CGAL_MAJOR_VERSION )

  set(CGAL_MINOR_VERSION)
  set(CGAL_BUGFIX_VERSION)
  set(CGAL_BUILD_VERSION 1000)

  if ( CGAL_VERSION_TOKENS_LEN GREATER 1 )
    list( GET CGAL_VERSION_TOKENS 1 CGAL_MINOR_VERSION )
  endif()

  if ( CGAL_VERSION_TOKENS_LEN GREATER 2 )
    list( GET CGAL_VERSION_TOKENS 2 CGAL_BUGFIX_VERSION )

    # The following condition will be true the token #2 is not a number (if
    # it is for example "I" or "Ic"):
    if(NOT CGAL_BUGFIX_VERSION GREATER 0 AND NOT CGAL_BUGFIX_VERSION EQUAL 0)
      set(CGAL_BUGFIX_VERSION 0)
      if( CGAL_VERSION_TOKENS_LEN GREATER 3 )
        list( GET CGAL_VERSION_TOKENS 3 CGAL_BUILD_VERSION )
      endif()
    else()
      if( CGAL_VERSION_TOKENS_LEN GREATER 4 )
        list( GET CGAL_VERSION_TOKENS 4 CGAL_BUILD_VERSION )
      endif()
    endif()
    # If CGAL_BUILD_VERSION is not a strictly positive number, error
    if(NOT CGAL_BUILD_VERSION GREATER 0)
      message(WARNING 
        "Error parsing VERSION file: CGAL_BUILD_VERSION is not a number.\n"
        "The content of the VERSION file is:\n${CGAL_VERSION}\n"
        "CGAL_BUILD_VERSION was set to:\n${CGAL_BUILD_VERSION}")
      set(CGAL_BUILD_VERSION 1000)
    endif()
  endif()

  if(NOT CGAL_BUGFIX_VERSION)
    set(CGAL_BUGFIX_VERSION 0)
  endif()

endif()

#--------------------------------------------------------------------------------------------------
#
#                                    -= BASIC SETUP =-
#
#--------------------------------------------------------------------------------------------------

message( STATUS "CGAL_MAJOR_VERSION=${CGAL_MAJOR_VERSION}" )
message( STATUS "CGAL_MINOR_VERSION=${CGAL_MINOR_VERSION}" )
message( STATUS "CGAL_BUGFIX_VERSION=${CGAL_BUGFIX_VERSION}" )
if(CGAL_BUILD_VERSION LESS 1000)
  message( STATUS "CGAL_BUILD_VERSION=${CGAL_BUILD_VERSION}" )
endif()

# SONAME, SOVERSION
#
# The rule is that each new release or bug fix release should increse the soversion.
#
# SOVERSION is $(SONAME_VERSION).$(SOVERSION_MINOR).$(SOVERSION_RELEASE)
#
# - If the binary interface of libraries do not change from previous release
#   (example: most bug fix releases), increase SOVERSION_RELEASE (this third number).
# - If the binary interface is changed, but remains compatible with
#   previous release (example: only addition of new functions), then increase
#   SOVERSION_MINOR (second number) and set SOVERSION_RELEASE to 0.
# - If the binary interface is changed in an incompatible way to previous
#   release, then increase the SONAME_VERSION, and set the two other
#   numbers to 0.
# 
# SOVERSION history: 
#   CGAL<=3.5  : (unversionned)
#   CGAL-3.5   : 4.0.0
#   CGAL-3.5.1 : 4.0.1
#   CGAL-3.6   : 5.0.0
#   CGAL-3.6.1 : 5.0.0 (should have been 5.0.1)
#   CGAL-3.7   : 6.0.0 (certainly: some types have been turned from int to
#                        std::size_t, which is different on some platforms)
#   CGAL-3.8   : 7.0.0 (At least CGAL::Random has changed its member fields.)
#   CGAL-3.9   : 8.0.0 (No way to check the binary compatibility.)
#   CGAL-4.0   : 9.0.0 (No way to check the binary compatibility.)
#   CGAL-4.1   : 10.0.0 (No way to check the binary compatibility.)
#   CGAL-4.2   : 10.0.1 (Nothing different in CGAL compiled libraries¹.)
#   CGAL-4.3   : 10.0.2 (Nothing different in CGAL compiled libraries¹.)
#   CGAL-4.4   : 10.0.3 (Nothing different in CGAL compiled libraries¹.)

# ¹) According to abi-http://upstream-tracker.org/versions/cgal.html

set( CGAL_SONAME_VERSION "10" )
set( CGAL_SOVERSION      "10.0.3" )

message( STATUS "CGAL_SONAME_VERSION=${CGAL_SONAME_VERSION}" )
message( STATUS "CGAL_SOVERSION     =${CGAL_SOVERSION}" )
set( CGAL_BUILDING_LIBS TRUE )

set( CGAL_VERSION_DIR     CGAL-${CGAL_VERSION} )
set( CGAL_MODULES_REL_DIR cmake/modules )
set( CGAL_MODULES_DIR     ${CGAL_INSTALLATION_PACKAGE_DIR}/${CGAL_MODULES_REL_DIR} )

include(${CGAL_MODULES_DIR}/CGAL_Macros.cmake)
cgal_setup_module_path()

message( STATUS "CGAL_REFERENCE_CACHE_DIR=${CGAL_REFERENCE_CACHE_DIR}" )

if ( RUNNING_CGAL_AUTO_TEST )
  message(STATUS "Operating system:")
  execute_process(COMMAND uname -a
    TIMEOUT 5
    OUTPUT_VARIABLE uname_a
    ERROR_VARIABLE uname_a)
  message(STATUS "${uname_a}")
  CGAL_display_compiler_version()
  if ( NOT "${CGAL_REFERENCE_CACHE_DIR}" STREQUAL "" )
    if ( EXISTS ${CGAL_REFERENCE_CACHE_DIR} )
      if ( EXISTS ${CGAL_REFERENCE_CACHE_DIR}/CMakeCache.txt )
        message( STATUS "Loading reference cache from ${CGAL_REFERENCE_CACHE_DIR}" )
        load_cache( ${CGAL_REFERENCE_CACHE_DIR} 
          EXCLUDE CGAL_Core_LIBRARY 
                  CGAL_CORE_PACKAGE_DIR
                  WITH_CGAL_Core
                  CGAL_INSTALLATION_PACKAGE_DIR
                  CGAL_MAINTENANCE_PACKAGE_DIR
                  CGAL_PDB_BINARY_DIR
                  CGAL_PDB_SOURCE_DIR
                  CGAL_Qt3_SOURCE_DIR
                  CGAL_BINARY_DIR
                  CGAL_SOURCE_DIR)
#        message("List of cache variables:")

        ## The following lines removes nasty loaded cache values. We do not
        ## want that the current build tree depends on binaries that were
        ## build in the reference build tree.
        get_property(cache_variables DIRECTORY PROPERTY CACHE_VARIABLES)
        foreach(var ${cache_variables}) 
#          get_property(var_value CACHE ${var} PROPERTY VALUE)
#          get_property(type CACHE ${var} PROPERTY TYPE)
          string(REGEX MATCH "^CGAL(_.*_(DEPENDS|BINARY_DIR)|_.*LIBRARY)$" var_name_matches ${var})
          if(var_name_matches)
            unset(${var} CACHE)
#          else()
#            message("${var}:${var_type}=${var_value}")
          endif()
        endforeach()


      endif()
    endif()
  endif()
endif()

include(CGAL_Common)
include(CGAL_GeneratorSpecificSettings)
include(CGAL_CheckCXXFileRuns)

mark_as_advanced(CMAKE_BACKWARDS_COMPATIBILITY)

message( STATUS "USING CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" )
message( STATUS "System: ${CMAKE_SYSTEM_NAME}" )

#--------------------------------------------------------------------------------------------------
#
#                                    -= FLAGS =-
#
#--------------------------------------------------------------------------------------------------

if( MSVC )
  
  uniquely_add_flags ( CGAL_CXX_FLAGS "-D_CRT_SECURE_NO_DEPRECATE;-D_SCL_SECURE_NO_DEPRECATE;-D_CRT_SECURE_NO_WARNINGS;-D_SCL_SECURE_NO_WARNINGS" )
  uniquely_add_flags ( CGAL_CXX_FLAGS "/fp:strict" )
  uniquely_add_flags ( CGAL_CXX_FLAGS "/fp:except-" )
  uniquely_add_flags ( CGAL_CXX_FLAGS "/wd4503" ) # Suppress warnings C4503 about "decorated name length exceeded"
  uniquely_add_flags ( CGAL_CXX_FLAGS "/bigobj" ) # Use /bigobj by default

  if ( RUNNING_CGAL_AUTO_TEST )
    set(CMAKE_CXX_WARNING_LEVEL 2 CACHE STRING "MSVC C++ compiler warning level" FORCE)
    mark_as_advanced(CMAKE_CXX_WARNING_LEVEL)
  endif()
    
endif()

if( "${CMAKE_CXX_COMPILER_ID}" MATCHES SunPro )
  message( STATUS "Using SunPro compiler, using STLPort 4." )
  
  uniquely_add_flags( CGAL_CXX_FLAGS "-features=extensions;-library=stlport4;-D_GNU_SOURCE" )
  uniquely_add_flags( CGAL_SHARED_LINKER_FLAGS "-library=stlport4" )
  uniquely_add_flags( CGAL_EXE_LINKER_FLAGS    "-library=stlport4" )
endif()

if( "${CMAKE_CXX_COMPILER}" MATCHES "icl" OR "${CMAKE_CXX_COMPILER}" MATCHES "icpc")
  message( STATUS "Intel compiler is detected." )
  set( IntelCompiler_FOUND TRUE )

  get_dependency_version(IntelCompiler)

  if( "${IntelCompiler_VERSION}" LESS "1100" )
    message("Intel Compiler version ${IntelCompiler_VERSION} is not supported by CGAL-${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}! (too old, must be 11.0 or after)")
  else()
    message( STATUS "Using Intel Compiler version 11 or later. Adding -fp-model strict" )
    if(WIN32)
      uniquely_add_flags( CGAL_CXX_FLAGS "/fp:strict" )
    else()
      uniquely_add_flags( CGAL_CXX_FLAGS "-fp-model strict" )
    endif()
  endif()
endif()


if ( CMAKE_COMPILER_IS_GNUCXX )

  set( GCC_FOUND TRUE )
  
  get_dependency_version(GCC)
  
  if ( "${GCC_VERSION}" MATCHES "Not" OR "${GCC_VERSION}" MATCHES "Unknown" )
    set( GCC_FOUND FALSE )
  endif()
  
  if ( GCC_FOUND )
  
    if ( RUNNING_CGAL_AUTO_TEST )
      uniquely_add_flags( CGAL_CXX_FLAGS "-Wall" )
    endif()
    
    if ( "${GCC_VERSION}" MATCHES "^4." )
      message( STATUS "Using gcc version 4 or later. Adding -frounding-math" )
      uniquely_add_flags( CGAL_CXX_FLAGS "-frounding-math" )
    endif()
    
    if ( "${GCC_VERSION}" MATCHES "^4.2" )
      message( STATUS "Using gcc version 4.2. Adding -fno-strict-aliasing" )
      uniquely_add_flags( CGAL_CXX_FLAGS "-fno-strict-aliasing" )
    endif()
    
    if ( "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "alpha" )
      message( STATUS "Using gcc on alpha. Adding -mieee -mfp-rounding-mode=d" )
      uniquely_add_flags( CGAL_CXX_FLAGS "-mieee -mfp-rounding-mode=d" )
    endif()
    
  endif()
  
endif()

message( "== Generate version files (DONE) ==\n")

#--------------------------------------------------------------------------------------------------
#
#                                    -= External libraries =-
#
#--------------------------------------------------------------------------------------------------
 
message("== Set up flags ==")

include(CGAL_SetupFlags)

message("== Set up flags (DONE) ==\n")

message("== Detect external libraries ==")

macro( add_config_flag flag )

    if ( ${flag} )
      file( APPEND ${CMAKE_BINARY_DIR}/include/CGAL/compiler_config.h "#define ${flag} 1\n\n")
    else()
      file( APPEND ${CMAKE_BINARY_DIR}/include/CGAL/compiler_config.h "//#define ${flag} 1\n\n")
    endif()

endmacro()
 
# this is the place to tell which external libs are supporting
# Remarks: 
#  External libs Qt3 and Qt34are configured when Qt3 or Qt4 as lib of cgal are required
#  Coin is used in KDS, but no FindCoin or FindCOIN exists
#  There exists FindF2C, FindIPE, FindMKL, but they are only used to support supporting libs
list (INSERT CGAL_SUPPORTING_3RD_PARTY_LIBRARIES 0 GMP MPFR ZLIB OpenGL LEDA MPFI RS RS3 OpenNL TAUCS Eigen3 BLAS LAPACK QGLViewer ESBTL Coin3D NTL IPE)
if (NOT WIN32)
  # GMPXX is not supported on WIN32 machines
  list (INSERT CGAL_SUPPORTING_3RD_PARTY_LIBRARIES 1 GMPXX)
endif()

# Where CMake is run several times, to avoid duplicates
list(REMOVE_DUPLICATES CGAL_SUPPORTING_3RD_PARTY_LIBRARIES)
hide_variable(CGAL_SUPPORTING_3RD_PARTY_LIBRARIES)

# set some to have special prefix
macro( set_special_prefix library prefix)
set(SPECIAL_PREFIXES "${SPECIAL_PREFIXES}set(CGAL_EXT_LIB_${library}_PREFIX \"${prefix}\")\n")
set(CGAL_EXT_LIB_${library}_PREFIX ${prefix})
endmacro()

set_special_prefix(Qt4 QT)
set_special_prefix(Eigen3 EIGEN3)
set_special_prefix(QGLViewer QGLVIEWER)
set_special_prefix(Coin3D COIN3D)

# some libraries are essential (stl and Boost.Thread are treated in another way) 
list (INSERT CGAL_ESSENTIAL_3RD_PARTY_LIBRARIES 0 GMP MPFR)
hide_variable(CGAL_ESSENTIAL_3RD_PARTY_LIBRARIES)

foreach (lib ${CGAL_SUPPORTING_3RD_PARTY_LIBRARIES}) 

  # set standard prefix
  if(NOT DEFINED CGAL_EXT_LIB_${lib}_PREFIX)
    set(CGAL_EXT_LIB_${lib}_PREFIX ${lib})
  endif()
  hide_variable(CGAL_EXT_LIB_${lib}_PREFIX)

  # add option
  list( FIND CGAL_ESSENTIAL_3RD_PARTY_LIBRARIES "${lib}" POSITION )
  if ( "${POSITION}" STRGREATER "-1" ) # if lib is essential
   option(WITH_${lib} "Select external library ${lib}" ON)
  else()
    option(WITH_${lib} "Select external library ${lib}" OFF)
  endif()

  if (WITH_${lib}) 
    add_config_flag( ${CGAL_EXT_LIB_${lib}_PREFIX} )
  endif()

endforeach()

#--------------------------------------------------------------------------------------------------
#
#                                    -= DEPENDENCIES =-
#
#--------------------------------------------------------------------------------------------------

#
# The following variables are in the cache just so subdirectories can set them persistently.
# But they are not intended to persist from run to run as normal cache variables.
# Similar variables are created when a library is detected.
#
cache_set(CGAL_3RD_PARTY_PRECONFIGURED  "" )

cache_set(CGAL_3RD_PARTY_DEFINITIONS    "" )
cache_set(CGAL_3RD_PARTY_INCLUDE_DIRS   "" )
cache_set(CGAL_3RD_PARTY_LIBRARIES      "" )
cache_set(CGAL_3RD_PARTY_LIBRARIES_DIRS "" ) 

# default is on, but some use-cases need to set it to off, e.g., debian packages
option( CGAL_ENABLE_PRECONFIG "Select to allow to preconfiguration of external libraries" ON)

# additional info: some header files in CGAL add additional code if
# certain optional libs are installed, and some examples/tests rely on
# this; e.g. in MPFI/RS in Algebraic_kernel_d. For these cases CGAL
# and the example/test must be configured with MPFI (just one is not sufficient)

option( CGAL_ALLOW_ALL_PRECONFIGURED_LIBS_COMPONENT "Select to allow to use all preconfigured external libraries" OFF)
 
include(CGAL_SetupDependencies)

message("== Detect external libraries (DONE) ==\n")

#--------------------------------------------------------------------------------------------------
#
#                                    -= Generation of compiler_config.h =-
#
#--------------------------------------------------------------------------------------------------

# The variables set are the #defines expected by compiler_config.h
# Note: CMake will not notice when files are added or removed
# but this is probably OK for the installation procedure.

message("== Write compiler_config.h ==")

file(GLOB all_config_tests "${CGAL_INSTALLATION_PACKAGE_DIR}/config/testfiles/*.cpp")

list( SORT all_config_tests )

file( WRITE ${CMAKE_BINARY_DIR}/include/CGAL/compiler_config.h "//\n// compiler_config.h is included by CGAL headers to load the needed compiler settings.\n//\n// DO NOT EDIT compiler_config.h. It is generated by CMake.\n//\n\n")

cache_get(CONFIG_CXX_FLAGS)

foreach(config_test_cpp ${all_config_tests})
    
    # Test's name is .cpp's base name
    get_filename_component(config_test_name ${config_test_cpp} NAME_WE)
    
    # Compile and run ${config_test_cpp}. Exit code is stored in ${config_test_name}.
    if( RUNNING_CGAL_AUTO_TEST OR "${config_test_name}" MATCHES "^${config_test_name}$" OR NOT ${CONFIG_CXX_FLAGS} STREQUAL  ${CMAKE_CXX_FLAGS})

      CHECK_CXX_FILE_RUNS(${config_test_cpp} ${config_test_name} ${config_test_name})

      if ( ${config_test_name} )
        cache_set ( ${config_test_name} 0 )
      else()
        cache_set ( ${config_test_name} 1 )
      endif()  
      
    endif()
    
    add_config_flag( ${config_test_name} ${config_test_name} )
    
endforeach()

cache_set(CONFIG_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_config_flag( CGAL_USE_GMP )
add_config_flag( CGAL_USE_MPFR )
add_config_flag( CGAL_USE_GMPXX )
add_config_flag( CGAL_USE_LEDA )
add_config_flag( CGAL_USE_MPFI )
add_config_flag( CGAL_USE_RS )
add_config_flag( CGAL_USE_NTL )

add_config_flag( CGAL_BUILD_SHARED_LIBS )

if (NOT ${WITH_CGAL_Core}) 
  set(CGAL_USE_CORE FALSE)
else()
  set(CGAL_USE_CORE ${CGAL_USE_GMP})
endif()
add_config_flag( CGAL_USE_CORE )
# ^^ there is CGAL_USE_CORE and not CGAL_HAS_CORE, as CORE is considered
# as external lib and not as CGAL component (EBEB: 24 Jan 2012)

if ( RUNNING_CGAL_AUTO_TEST AND MSVC )
  file( APPEND "${CMAKE_BINARY_DIR}/include/CGAL/compiler_config.h" "#include <CGAL/Testsuite/vc_debug_hook.h>\n\n" )
endif()

#--------------------------------------------------------------------------------------------------
#
#                                    -= Installation Setup =-
#
#--------------------------------------------------------------------------------------------------

set ( CGAL_INSTALL_INC_DIR "include" CACHE STRING "The folder where CGAL header files will be installed, relative to CMAKE_INSTALL_PREFIX" )
set ( CGAL_INSTALL_LIB_DIR "lib"     CACHE STRING "The folder where CGAL libraries will be installed, relative to CMAKE_INSTALL_PREFIX" )

if ( CGAL_WIN32_CMAKE_ON_CYGWIN )
  exec_program(cygpath ARGS -w "${CMAKE_INSTALL_PREFIX}" OUTPUT_VARIABLE CMAKE_INSTALL_PREFIX2 )
  file ( TO_CMAKE_PATH ${CMAKE_INSTALL_PREFIX2} CMAKE_INSTALL_PREFIX )
endif()

set ( CGAL_INSTALL_BIN_DIR    "bin"                     
  CACHE STRING "The folder where CGAL user-side scripts will be installed, relative to CMAKE_INSTALL_PREFIX" 
  )

set ( CGAL_INSTALL_CMAKE_DIR "${CGAL_INSTALL_LIB_DIR}/CGAL"
  CACHE STRING "The folder where CGAL CMake modules will be installed, relative to CMAKE_INSTALL_PREFIX" 
  )

set ( CGAL_INSTALL_DOC_DIR "share/doc/${CGAL_VERSION_DIR}"  
  CACHE STRING "The folder where CGAL documentation and license files will be installed, relative to CMAKE_INSTALL_PREFIX" 
  )

set ( CGAL_INSTALL_MAN_DIR "share/man/man1"  
  CACHE STRING "The folder where manual pages for CGAL scripts will be installed, relative to CMAKE_INSTALL_PREFIX" 
  )

message("== Write compiler_config.h (DONE) ==\n")

#--------------------------------------------------------------------------------------------------
#
#                                    -= Build =-
#
#--------------------------------------------------------------------------------------------------

message("== Generating build files ==")

set(CGAL_LIBRARIES_DIR ${CMAKE_BINARY_DIR}/lib)

set(CGAL_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/include)

foreach (package ${CGAL_CONFIGURED_PACKAGES})
  set(CGAL_INCLUDE_DIRS ${CGAL_INCLUDE_DIRS} ${package}/include)
endforeach()

include_directories (${CGAL_INCLUDE_DIRS})

cache_get(CGAL_LIBRARY)
cache_get(CGAL_3RD_PARTY_PRECONFIGURED )

cache_get(CGAL_3RD_PARTY_DEFINITIONS   )
cache_get(CGAL_3RD_PARTY_INCLUDE_DIRS  )
cache_get(CGAL_3RD_PARTY_LIBRARIES     )
cache_get(CGAL_3RD_PARTY_LIBRARIES_DIRS) 

add_subdirectory(src)

hide_variable(EXECUTABLE_OUTPUT_PATH)
hide_variable(LIBRARY_OUTPUT_PATH)

#
# Repeat some problems
#
message("== Generating build files (DONE) ==\n")

if (CGAL_BRANCH_BUILD AND CGAL_REPORT_DUPLICATE_FILES )

  message( STATUS "Problems: ")

  if (CGAL_DUPLICATE_HEADER_FILES)
    message(WARNING "WARNING: Duplicate header files")
    foreach(hfile ${CGAL_DUPLICATE_HEADER_FILES})
      message(STATUS "  File '${hfile}' multiply found in ")
      foreach (package ${CGAL_CONFIGURED_PACKAGES})
        file(GLOB_RECURSE CGAL_PACKAGE_HEADER_FILES FOLLOW_SYMLINKS ${package}/*.h)
        foreach (file ${CGAL_PACKAGE_HEADER_FILES})
          string(REPLACE "${package}/" "" pure_file ${file})
          if ("${pure_file}" STREQUAL "${hfile}")
            message(STATUS "    ${package}")
          endif()
        endforeach()
      endforeach()
    endforeach()
  endif()

  if (CGAL_DUPLICATE_CPP_FILES)
    message(WARNING "WARNING: Duplicate cpp files")
    foreach(cfile ${CGAL_DUPLICATE_CPP_FILES})
      message(STATUS "  File '${cfile}' multiply found in ")
      foreach (package ${CGAL_CONFIGURED_PACKAGES})
        file(GLOB_RECURSE CGAL_PACKAGE_CPP_FILES FOLLOW_SYMLINKS ${package}/*.cpp)
        foreach (file ${CGAL_PACKAGE_CPP_FILES})
          string(REPLACE "${package}/" "" pure_file ${file})
          if ("${pure_file}" STREQUAL "${cfile}")
            message(STATUS "    ${package}")
          endif()
        endforeach()
      endforeach()
    endforeach()
  endif()

endif()

#--------------------------------------------------------------------------------------------------
#
#                                    -= USER SIDE SETTINGS =-
#
#--------------------------------------------------------------------------------------------------

# Set CGAL_LIBRARY_INSTALLED, CGAL_Qt3_LIBRARY_INSTALLED and so on. Those
# variables are the path to the libraries installed by the install
# target. Used in the generation of the installed CGALConfig.cmake
foreach(lib ${CGAL_CONFIGURED_LIBRARIES} CGAL) # note the added fake 'CGAL'
 if(${lib}_LIBRARY)
    get_filename_component(lib_name "${${lib}_LIBRARY}" NAME)
    set(${lib}_LIBRARY_INSTALLED "${CMAKE_INSTALL_PREFIX}/${CGAL_INSTALL_LIB_DIR}/${lib_name}")
 else()
    set(${lib}_LIBRARY_INSTALLED "")
  endif()
  hide_variable(${lib}_LIBRARY_INSTALLED)
endforeach()

create_CGALconfig_files()

#--------------------------------------------------------------------------------------------------
#
#                                    -= Installation Commands =-
#
#--------------------------------------------------------------------------------------------------

# DESTINATION option is mandatory; skipping it breaks CPack!

if(CGAL_INSTALL_DOC_DIR)
  install(FILES AUTHORS CHANGES LICENSE LICENSE.FREE_USE LICENSE.GPL LICENSE.LGPL DESTINATION ${CGAL_INSTALL_DOC_DIR} )
endif()

#install all includes collected in trunk et cetera


foreach (dir ${CGAL_CONFIGURED_PACKAGES})
  if (EXISTS ${dir}/include/CGAL) 
    install(DIRECTORY ${dir}/include/CGAL              DESTINATION ${CGAL_INSTALL_INC_DIR} PATTERN ".svn" EXCLUDE)
  endif()
endforeach()
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/CGAL     DESTINATION ${CGAL_INSTALL_INC_DIR} PATTERN ".svn" EXCLUDE)

file(GLOB scripts "scripts/*")
list(REMOVE_ITEM scripts 
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cgal_create_assertions.sh 
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cgal_create_cmake_script_with_options)
install(PROGRAMS ${scripts} DESTINATION ${CGAL_INSTALL_BIN_DIR})

install(DIRECTORY ${CGAL_MODULES_REL_DIR}/                DESTINATION ${CGAL_INSTALL_CMAKE_DIR} )
install(FILES     ${CGAL_MODULES_REL_DIR}/UseCGAL.cmake   DESTINATION ${CGAL_INSTALL_CMAKE_DIR} )


if ( GMP_IN_AUXILIARY )
  install(DIRECTORY auxiliary/gmp/include/ DESTINATION ${CGAL_INSTALL_INC_DIR} )
  install(DIRECTORY auxiliary/gmp/lib/     DESTINATION ${CGAL_INSTALL_LIB_DIR} )
endif()

if ( TAUCS_IN_AUXILIARY )
  install(DIRECTORY auxiliary/taucs/include/ DESTINATION ${CGAL_INSTALL_INC_DIR} )
  install(DIRECTORY auxiliary/taucs/lib/     DESTINATION ${CGAL_INSTALL_LIB_DIR} )
endif()

if ( ZLIB_IN_AUXILIARY )
  install(DIRECTORY auxiliary/zlib/include/ DESTINATION ${CGAL_INSTALL_INC_DIR} )
  install(DIRECTORY auxiliary/zlib/lib/     DESTINATION ${CGAL_INSTALL_LIB_DIR} )
endif()


install(FILES ${CMAKE_BINARY_DIR}/config/CGALConfig.cmake DESTINATION ${CGAL_INSTALL_CMAKE_DIR} )

if(CGAL_INSTALL_MAN_DIR)
  install(FILES auxiliary/cgal_create_cmake_script.1 DESTINATION ${CGAL_INSTALL_MAN_DIR} )
endif()

#--------------------------------------------------------------------------------------------------
#
#              -= Special installation commands to copy FindCGAL to the cmake directory =-
#
#--------------------------------------------------------------------------------------------------

# Installing FindCGAL into cmake itself is implemented as a custom target
# because it requires superuser privileges

if ( NOT "${CMAKE_ROOT}" STREQUAL "" )
  if ( EXISTS ${CMAKE_ROOT}/Modules )
    add_custom_target( install_FindCGAL
                      COMMAND ${CMAKE_COMMAND} -E copy ${CGAL_MODULES_DIR}/FindCGAL.cmake ${CMAKE_ROOT}/Modules
                    ) 
  endif()
endif()  

#--------------------------------------------------------------------------------------------------
#
#                                    -= APPLICATIONS =-
#
#--------------------------------------------------------------------------------------------------

macro( add_programs subdir target ON_OFF )

  cache_set( CGAL_EXECUTABLE_TARGETS "" )
  
  add_custom_target( ${target} )
  
  option( WITH_${target} "Select ${target}" ${ON_OFF} )
  if ( WITH_${target} )
    add_subdirectory( ${subdir} EXCLUDE_FROM_ALL )
  endif()
  
  cache_get( CGAL_EXECUTABLE_TARGETS )
  
  foreach( CGAL_EXECUTABLE_TARGET ${CGAL_EXECUTABLE_TARGETS} )
    add_dependencies( ${target} ${CGAL_EXECUTABLE_TARGET} )
  endforeach()
     
endmacro()

# This allows programs to locate CGALConfig.cmake
set(CGAL_DIR ${CMAKE_BINARY_DIR} )

if ( NOT RUNNING_CGAL_AUTO_TEST )

  add_programs(examples examples OFF )
  add_programs(demo     demos    OFF )
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_programs(test     tests    OFF )
  endif()
endif()

#--------------------------------------------------------------------------------------------------
#
#                                            -= CPack =-
#
#--------------------------------------------------------------------------------------------------

#option( WITH_CPACK "Create package generation rules")
if( WITH_CPACK AND EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake" )

    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CGAL - Computational Geometry Algorithms Library")
    set(CPACK_PACKAGE_VENDOR "CGAL Open Source Project")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CGAL_INSTALLATION_PACKAGE_DIR}/README")
    set(CPACK_RESOURCE_FILE_LICENSE "${CGAL_INSTALLATION_PACKAGE_DIR}/LICENSE")
    set(CPACK_PACKAGE_VERSION_MAJOR "${CGAL_MAJOR_VERSION}")
    set(CPACK_PACKAGE_VERSION_MINOR "${CGAL_MINOR_VERSION}")
    set(CPACK_PACKAGE_VERSION_PATCH "${CGAL_BUGFIX_VERSION}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CGAL ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "CGAL-${CGAL_VERSION}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CGAL_INSTALLATION_PACKAGE_DIR}/LICENSE")
    
    if(NOT DEFINED CPACK_SYSTEM_NAME)
      set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
    endif()
    
    if(${CPACK_SYSTEM_NAME} MATCHES Windows)
      if(CMAKE_CL_64)
        set(CPACK_SYSTEM_NAME win64-${CMAKE_SYSTEM_PROCESSOR})
      else()
        set(CPACK_SYSTEM_NAME win32-${CMAKE_SYSTEM_PROCESSOR})
      endif()
    endif()
    
    if(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
      set(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
    endif()
    
    set(CPACK_PACKAGE_EXECUTABLES "CGAL" "CGAL")

    if(WIN32 AND NOT UNIX)
        set(CPACK_GENERATOR "NSIS")
        # There is a bug in NSI that does not handle full unix paths properly. Make
        # sure there is at least one set of four (4) backlasshes.
        #set(CPACK_PACKAGE_ICON "${CGAL_INSTALLATION_PACKAGE_DIR}\\\\cgal_install.gif")
        #set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\CGAL.exe")
        set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} Computational Geometry Algorithms Library")
        set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.cgal.org")
        set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.cgal.org")
        set(CPACK_NSIS_CONTACT "info@cgal.org")
        set(CPACK_NSIS_MODIfY_PATH ON)
    else()
        set(CPACK_STRIP_FILES "")
        set(CPACK_SOURCE_STRIP_FILES "")
    endif()

    include(CPack)
    
endif()


if ( CGAL_BRANCH_BUILD )
  option(CGAL_ENABLE_CHECK_HEADERS 
    "Enable the special targets \"check_pkg_headers\", and \"check_pkg_<package>_headers\" for each package"
    FALSE)

  if(CGAL_ENABLE_CHECK_HEADERS)

    if(NOT DEFINED CGAL_CHECK_SYNTAX_ONLY)
      execute_process(COMMAND 
        ${CMAKE_CXX_COMPILER} -x c++ -fsyntax-only ${CMAKE_CURRENT_SOURCE_DIR}/config/support/test_syntaxonly.cpp
        ERROR_QUIET
        RESULT_VARIABLE ok)
      if(ok EQUAL 0)
        set(CGAL_CHECK_SYNTAX_ONLY ON CACHE INTERNAL "")
      else()
        set(CGAL_CHECK_SYNTAX_ONLY OFF CACHE INTERNAL "")
      endif()
    endif(NOT DEFINED CGAL_CHECK_SYNTAX_ONLY)

    if(NOT CGAL_CHECK_SYNTAX_ONLY)
      message(FATAL_ERROR "Your compiler does not seem to support -fsyntax-only.
You must disable CGAL_ENABLE_CHECK_HEADERS.")
    endif()

    ## Fill the variable include_options with all the -I and -isystem options
    set(include_options)
    foreach (incdir ${CGAL_INCLUDE_DIRS})
      list(APPEND include_options "-I${incdir}")
    endforeach()
    foreach (incdir ${CGAL_3RD_PARTY_INCLUDE_DIRS})
      list(APPEND include_options "-isystem${incdir}")
    endforeach()
    include_directories ( SYSTEM ${CGAL_3RD_PARTY_INCLUDE_DIRS} )

    ## Loop on package and headers
    set(check_pkg_target_list)
    foreach (package ${CGAL_CONFIGURED_PACKAGES_NAMES})
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../${package}/include)
        set(depends "")
        file(GLOB ${package}_HEADERS 
          RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../${package}/include" 
          "${CMAKE_CURRENT_SOURCE_DIR}/../${package}/include/CGAL/*.h")
        foreach(header ${${package}_HEADERS})
          string(REPLACE "/" "__" header2 "${header}")
          string(REPLACE "." "_" header2 "${header2}")
          add_custom_command(OUTPUT check_${header2}
            COMMAND ${CMAKE_CXX_COMPILER} ${CGAL_3RD_PARTY_DEFINITIONS} ${CGAL_DEFINITIONS} ${include_options} -x c++ -fsyntax-only "${CMAKE_CURRENT_SOURCE_DIR}/../${package}/include/${header}"
            COMMAND ${CMAKE_COMMAND} -E touch check_${header2}
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../${package}/include/${header}"
            VERBATIM
            COMMENT "Check header ${header}"
            )
          list(APPEND depends check_${header2})
        endforeach() # look on headers
        add_custom_target(check_pkg_${package}_headers DEPENDS ${depends})
        list(APPEND check_pkg_target_list check_pkg_${package}_headers)
      endif() # if the package has an include directory
    endforeach() # loop on packages
    add_custom_target(check_headers DEPENDS ${check_pkg_target_list})
  endif()
endif( CGAL_BRANCH_BUILD )

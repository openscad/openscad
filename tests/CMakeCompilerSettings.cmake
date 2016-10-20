# Compiler settings for test build, see CMakeLists.txt, doc/testing.txt

# Detect Lion and force gcc
IF (APPLE)
   EXECUTE_PROCESS(COMMAND sw_vers -productVersion OUTPUT_VARIABLE MACOSX_VERSION)
   IF (NOT ${MACOSX_VERSION} VERSION_LESS "10.9.0")
     message("Detected Maverick (10.9) or later")
     set(CMAKE_C_COMPILER "clang")
     set(CMAKE_CXX_COMPILER "clang++")
     # Somehow, since we build dependencies for 10.7, we need to also build executables
     # for 10.7. This used to not be necessary, but since 10.9 it apparently is..
     SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.7 CACHE STRING "Deployment target")
   ELSEIF (NOT ${MACOSX_VERSION} VERSION_LESS "10.8.0")
     message("Detected Mountain Lion (10.8)")
     set(CMAKE_C_COMPILER "clang")
     set(CMAKE_CXX_COMPILER "clang++")
   ELSEIF (NOT ${MACOSX_VERSION} VERSION_LESS "10.7.0")
     message("Detected Lion (10.7)")
     set(CMAKE_C_COMPILER "clang")
     set(CMAKE_CXX_COMPILER "clang++")
   ELSE()
     message("Detected Snow Leopard (10.6) or older")
     if (USE_LLVM)
       message("Using LLVM compiler")
       set(CMAKE_C_COMPILER "llvm-gcc")
       set(CMAKE_CXX_COMPILER "llvm-g++")
     endif()
   ENDIF()
ENDIF(APPLE)

# Get GCC version
if(CMAKE_COMPILER_IS_GNUCXX)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
endif()

# Build debug build as default
if(NOT CMAKE_BUILD_TYPE)
  #  set(CMAKE_BUILD_TYPE Release)
  if(CMAKE_COMPILER_IS_GNUCXX)
    if (GCC_VERSION VERSION_GREATER 4.6)
      set(CMAKE_BUILD_TYPE RelWithDebInfo)
    else()
      set(CMAKE_BUILD_TYPE Debug)
    endif()
  else()
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
else()
  # asserts will be compiled out with NDEBUG -- must remove it
  foreach (flags_var_to_scrub
           CMAKE_CXX_FLAGS_RELEASE
           CMAKE_CXX_FLAGS_RELWITHDEBINFO
           CMAKE_CXX_FLAGS_MINSIZEREL
           CMAKE_C_FLAGS_RELEASE
           CMAKE_C_FLAGS_RELWITHDEBINFO
           CMAKE_C_FLAGS_MINSIZEREL)
    string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " "
           "${flags_var_to_scrub}" "${${flags_var_to_scrub}}")
  endforeach()
endif()


#
# Handle C++11.
# Note: Since Xcode 7, c++11 is default

message(STATUS "Using C++11")
  if (NOT CMAKE_COMPILER_IS_GNUCXX OR GCC_VERSION VERSION_GREATER 4.6)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()
if(APPLE)
  set(LIBCXX11 1)
  message(STATUS "Using libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()


#
# Windows(TM)
# Note - if(WIN32) will also detect 64-bit Win

if(WIN32)
  # Unicode in Windows(TM) API, Windows Data Types for Strings
  # https://msdn.microsoft.com/en-us/library/windows/desktop/dd374131(v=vs.85).aspx
  add_definitions( -DUNICODE )
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -frounding-math")
  endif()
endif()

# Clang compiler

if(${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
  # disable enormous amount of warnings about CGAL
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

	
if(${CMAKE_CXX_COMPILER} MATCHES ".*clang.*" AND NOT ${CGAL_CXX_FLAGS_INIT} STREQUAL "" )
  string(REPLACE "-frounding-math" "" CGAL_CXX_FLAGS_INIT ${CGAL_CXX_FLAGS_INIT})
  string(REPLACE "--param=ssp-buffer-size=4" "" CGAL_CXX_FLAGS_INIT ${CGAL_CXX_FLAGS_INIT})
  # clang fails to build several included standard C++ libs on some
  # machines when FORTIFY_SOURCE is enabled.
  message(STATUS "disabling FORTIFY_SOURCE: https://bugzilla.redhat.com/show_bug.cgi?id=1188075")
  string(REPLACE "FORTIFY_SOURCE=2" "FORTIFY_SOURCE=0" CGAL_CXX_FLAGS_INIT ${CGAL_CXX_FLAGS_INIT})
endif()


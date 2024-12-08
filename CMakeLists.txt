#
# CMake build system for OpenSCAD
#
# Configuration variables
#   -DHEADLESS=<ON|OFF>
#   -DNULLGL=<ON|OFF>
#   -DSNAPSHOT=<ON|OFF>
#   -DEXPERIMENTAL=<ON|OFF>
#

cmake_minimum_required(VERSION 3.13)

if(POLICY CMP0071) # Let AUTOMOC and AUTOUIC process GENERATED files.
  cmake_policy(SET CMP0071 NEW)
endif()
if(POLICY CMP0072) # FindOpenGL prefers GLVND by default when available.
  cmake_policy(SET CMP0072 NEW)
endif()
if(POLICY CMP0079) # `target_link_libraries()` allows use with targets in other directories.
  cmake_policy(SET CMP0079 NEW)
endif()
if(POLICY CMP0082) # Install rules from add_subdirectory() calls are interleaved with those in caller.
  cmake_policy(SET CMP0082 NEW)
endif()
if(POLICY CMP0077) # Let parent project set child project options (used to skip python lookup from manifold & force TBB)
  cmake_policy(SET CMP0079 NEW)
endif()

include("cmake/Modules/openscad_version.cmake")

option(INFO "Display build configuration info at end of cmake config" ON)
option(ENABLE_TESTS "Run testsuite after building." ON)
option(EXPERIMENTAL "Enable Experimental Features" OFF)
option(USE_MANIFOLD_TRIANGULATOR "Use Manifold's triangulator instead of CGAL's" ON)
option(USE_BUILTIN_MANIFOLD "Use manifold from submodule" ON)
option(USE_LEGACY_RENDERERS "Use legacy (non-VBO) OpenGL renderers" OFF)
option(SNAPSHOT "Create dev snapshot, uses nightly icons" OFF)
option(HEADLESS "Build without GUI frontend" OFF)
if(APPLE)
  option(USE_QT6 "Build the GUI with Qt6" ON)
else()
option(USE_QT6 "Build the GUI with Qt6" OFF)
endif()
option(ENABLE_EGL "Enable EGL support if available" ON)
option(ENABLE_GLX "Enable GLX support if available" ON)
# For now, we'll default to whatever OpenCSG uses (>=1.6 -> GLAD, <1.6 -> GLEW)
option(USE_GLAD "Use GLAD. Mutually exclusive from USE_GLEW" OFF)
option(USE_GLEW "Use GLEW. Mutually exclusive from USE_GLAD" OFF)
option(NULLGL "Build without OpenGL, (implies HEADLESS=ON) " OFF)
option(IDPREFIX "Prefix CSG nodes with index # (debugging purposes only, will break node cache)" OFF)
option(PROFILE "Enable compiling with profiling / test coverage instrumentation" OFF)
option(MXECROSS "Enable setup for MXE cross platform build" OFF)
option(OFFLINE_DOCS "Download Documentation for offline usage" OFF)
option(USE_MIMALLOC "Use mimalloc as malloc replacement." ON)
option(USE_BUILTIN_OPENCSG "Use OpenCSG from submodule." OFF)
option(USE_CCACHE "Use ccache to speed up compilation." ON)
option(ENABLE_CAIRO "Enable support for cairo vector graphics library." ON)
option(ENABLE_SPNAV "Enable support for libspnav input driver." ON)
option(ENABLE_HIDAPI "Enable support for HIDAPI input driver." ON)
option(ENABLE_CGAL "Enable CGAL." ON)
option(ENABLE_MANIFOLD "Enable Manifold." ON)
option(ALLOW_BUNDLED_HIDAPI "Allow usage of bundled HIDAPI library (Windows only)." OFF)
option(ENABLE_PYTHON "Enable experimental Python Interpreter" OFF)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
include(CMakeDependentOption)
cmake_dependent_option(APPLE_UNIX "Build OpenSCAD in Unix mode in MacOS X instead of an Apple Bundle" OFF "APPLE" OFF)
cmake_dependent_option(ENABLE_QTDBUS "Enable DBus input driver for Qt." ON "NOT HEADLESS" OFF)
cmake_dependent_option(ENABLE_GAMEPAD "Enable Qt5Gamepad input driver." ON "NOT HEADLESS;NOT USE_QT6" OFF)

if (USE_QT6)
  # Minimum version required for Qt-6.5
  set(DEPLOYMENT_TARGET "11.0")
else()
  # Minimum version required for std::filesystem
  set(DEPLOYMENT_TARGET "10.15")
endif()

# Note: CMAKE_OSX_DEPLOYMENT_TARGET must be set before the project() invocation
set(CMAKE_OSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET} CACHE STRING "Minimum OS X deployment version")

project(OpenSCAD
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "The Programmer's Solid 3D CAD Modeler"
  HOMEPAGE_URL "https://openscad.org/"
  LANGUAGES C CXX
)

# GNUInstallDirs must be done before BuildParameters
include(GNUInstallDirs)

# Use clang-tidy if run with -DCLANG_TIDY=1
set(CLANG_TIDY ${CLANG_TIDY} CACHE BOOL "Enable clang-tidy")
if(CLANG_TIDY)
  if(APPLE)
    # clang-tidy isn't directly available on Homebrew, but exists inside the llvm package
    set(CLANG_TIDY_HINTS "/opt/homebrew/opt/llvm/bin" "/usr/local/opt/llvm/bin")
  endif()
  find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED HINTS ${CLANG_TIDY_HINTS})
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
  include("cmake/Modules/RegexUtils.cmake")
  escape_string_as_regex(regex "${CMAKE_SOURCE_DIR}/src/")
  # /src/ext/.clang-tidy disables all checks for that dir.  Copy into build dir to ignore generated sources.
  configure_file(${CMAKE_SOURCE_DIR}/src/ext/.clang-tidy ${CMAKE_BINARY_DIR} COPYONLY)
  # Regex hack below since negative lookahead is not supported (POSIX regex only)
  #  (?!ext/) becomes ([^e]...|e[^x]..|ex[^t].|ext[^/]|.{0,3}$)
  # CMAKE_CXX_CLANG_TIDY must be set before target is created (with add_executable())
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--header-filter=${regex}([^e]...|e[^x]..|ex[^t].|ext[^/]|.{0,3}$)")
  #  append ";--fix" to arguments above to apply automatic fix (if the given checks support it)
endif(CLANG_TIDY)

add_executable(OpenSCAD)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/submodules/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

# Strict c++17, e.g. no GNU extensions
set_property(TARGET OpenSCAD PROPERTY CXX_STANDARD 17)
set_property(TARGET OpenSCAD PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET OpenSCAD PROPERTY CXX_STANDARD_REQUIRED ON)
# Output compilation database (compile_commands.json), so we can e.g. run clang-tidy or other tools separately
set_property(TARGET OpenSCAD PROPERTY EXPORT_COMPILE_COMMANDS ON)
set_property(TARGET OpenSCAD PROPERTY LINKER_LANGUAGE CXX)
# Verbose link step
if(("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "GNU") OR
   ("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang|Clang|GNU"))
  target_link_options(OpenSCAD BEFORE PRIVATE "-v")
endif()

set(SUFFIX "" CACHE STRING "Installation suffix for binary (e.g. 'nightly')")
set(STACKSIZE 8388608 CACHE STRING "Stack size (default is 8MB)")

if(PROFILE)
  SET(GCC_COVERAGE_COMPILE_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage -fprofile-dir=.gcov")
  SET(GCC_COVERAGE_LINK_FLAGS    "--coverage")
  # TODO use more specific commands "target_compile_options" etc. to avoid potentially affecting submodules?
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
  set(Boost_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
  set(Boost_USE_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
  set(ENABLE_CAIRO OFF CACHE BOOL "" FORCE)
  set(MANIFOLD_PAR OFF CACHE STRING "" FORCE)
  set(NULLGL ON CACHE BOOL "" FORCE)
  set(USE_MIMALLOC OFF CACHE BOOL "" FORCE)
  set(ENV{PKG_CONFIG_PATH} "/emsdk/upstream/emscripten/cache/sysroot/lib/pkgconfig")
  target_compile_definitions(OpenSCAD PRIVATE CGAL_DISABLE_ROUNDING_MATH_CHECK)
  target_link_libraries(OpenSCAD PRIVATE /emsdk/upstream/emscripten/cache/sysroot/lib/libz.a)
  target_link_options(OpenSCAD PUBLIC
    -sALLOW_MEMORY_GROWTH=1
    -sENVIRONMENT=web,worker
    -sEXIT_RUNTIME=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME=OpenSCAD
    -sEXPORTED_RUNTIME_METHODS=['FS','ENV','callMain','ERRNO_CODES']
    -sFORCE_FILESYSTEM=1
    -sMODULARIZE=1
    -sNO_DISABLE_EXCEPTION_CATCHING
    -sUSE_PTHREADS=0
  )
endif()

if(NULLGL)
  set(HEADLESS ON CACHE BOOL "" FORCE)
endif()
if(EXPERIMENTAL)
  target_compile_definitions(OpenSCAD PRIVATE ENABLE_EXPERIMENTAL)
endif()
if(IDPREFIX)
  target_compile_definitions(OpenSCAD PRIVATE IDPREFIX)
endif()
if(USE_LEGACY_RENDERERS)
  target_compile_definitions(OpenSCAD PRIVATE USE_LEGACY_RENDERERS)
endif()

if (NOT "${SUFFIX}" STREQUAL "")
  set(SUFFIX_WITH_DASH "-${SUFFIX}")
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_SUFFIX="${SUFFIX_WITH_DASH}")
endif()

if (MXECROSS OR WIN32)
  add_subdirectory(winconsole)
  set_property(TARGET OpenSCAD PROPERTY WIN32_EXECUTABLE ON)
endif()

set(OPENSCAD_LIB_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/objects")
file(MAKE_DIRECTORY ${OPENSCAD_LIB_OUTPUT_DIR})

# Default to Release build
#if(NOT CMAKE_BUILD_TYPE)
#  message(STATUS "CMAKE_BUILD_TYPE not specified.  Defaulting to 'Release'")
#  message(STATUS "Usage: cmake -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel] .")
#  set(CMAKE_BUILD_TYPE Release)
#else()
#  message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
#endif()

target_compile_options(OpenSCAD PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUG>")

target_compile_definitions(OpenSCAD PRIVATE
  _REENTRANT
  UNICODE
  _UNICODE
  _USE_MATH_DEFINES)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frounding-math")
  if (WIN32) # gcc bug spams warnings, See issue #2771
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
  endif()
endif()

find_package(PkgConfig)

add_subdirectory(submodules)

# The submodules will set up mimalloc if needed, and we need to defer any
# target_link_libraries() calls until after that.

if (MXECROSS)
  target_compile_definitions(OpenSCAD PRIVATE LIBXML_STATIC STATIC_QT_SVG_PLUGIN)
  target_compile_definitions(OpenSCAD PRIVATE GLEW_STATIC)
  target_link_libraries(OpenSCAD PRIVATE bcrypt)
endif()

macro(find_graphics)
  # NULLGL - Allow us to build without OpenGL(TM). run 'cmake .. -DNULLGL=1'
  # Most tests will fail, but it can be used for testing/experiments
  if(NULLGL)
    target_compile_definitions(OpenSCAD PRIVATE NULLGL)
  else()

    set(OPENSCAD_UNITY_BUILD ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)
    if(NOT USE_BUILTIN_OPENCSG)
      find_package(OpenCSG REQUIRED QUIET)
      target_link_libraries(OpenSCAD PRIVATE ${OPENCSG_LIBRARY})
      message(STATUS "OpenCSG: ${OPENCSG_VERSION_STRING}")
      if(MSVC)
        find_path(OPENCSG_INCLUDE_DIRS opencsg/opencsg.h)
        target_include_directories(OpenSCAD SYSTEM PRIVATE "${OPENCSG_INCLUDE_DIRS}/opencsg")
      else()
        find_path(OPENCSG_INCLUDE_DIRS opencsg.h)
        target_include_directories(OpenSCAD SYSTEM PRIVATE ${OPENCSG_INCLUDE_DIRS})
      endif()
    else()
      include("submodules/CMakeLists-OpenCSG.txt")
      target_link_libraries(OpenSCAD PRIVATE OpenCSG)
      message(STATUS "OpenCSG submodule: ${OPENCSG_VERSION_STRING}")
    endif(NOT USE_BUILTIN_OPENCSG)
    set(CMAKE_UNITY_BUILD ${OPENSCAD_UNITY_BUILD})

    target_compile_definitions(OpenSCAD PRIVATE ENABLE_OPENCSG)

    if (${OPENCSG_VERSION_STRING} VERSION_LESS "1.6.0")
      set(OPENCSG_GLEW ON)
      target_compile_definitions(OpenSCAD PRIVATE OPENCSG_GLEW)
    endif()

    if (NOT USE_GLAD AND NOT USE_GLEW)
      if (OPENCSG_GLEW)
        set(USE_GLEW ON)
        set(WRANGLER "glew")
      else()
	set(USE_GLAD ON)
        set(WRANGLER "GLAD")
      endif()
      message(STATUS "USE_GLAD/USE_GLEW not specified: Defaulting to ${WRANGLER}")
    endif()

    if (USE_GLEW OR OPENCSG_GLEW)
      find_package(GLEW REQUIRED QUIET)
      message(STATUS "GLEW: ${GLEW_VERSION}")
      target_link_libraries(OpenSCAD PRIVATE GLEW::glew)
      set(GLEW_SOURCES src/glview/glew-utils.cc)
    endif()

    find_package(OpenGL REQUIRED QUIET)
    target_link_libraries(OpenSCAD PRIVATE ${OPENGL_LIBRARIES})
    message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
  endif()
endmacro(find_graphics)

if (MSVC)
  # Flex lexer options
  set(WINCOMPAT "--wincompat --nounistd")
  target_compile_definitions(OpenSCAD PRIVATE _USE_MATH_DEFINES)

  find_package(Eigen3 CONFIG REQUIRED)
  target_link_libraries(OpenSCAD PRIVATE Eigen3::Eigen)
  message(STATUS "Eigen: ${Eigen3_VERSION}")

  set(Boost_USE_STATIC_LIBS TRUE)
  find_package(Boost 1.61 REQUIRED COMPONENTS system regex program_options)
  message(STATUS "Boost: ${Boost_VERSION}")
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(OpenSCAD PRIVATE ${Boost_LIBRARIES})
  target_compile_definitions(OpenSCAD PRIVATE BOOST_DLL_USE_STD_FS)

  find_package(harfbuzz CONFIG REQUIRED)
  find_path(HARFBUZZ_INCLUDE_DIRS harfbuzz)
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${HARFBUZZ_INCLUDE_DIRS}/harfbuzz)
  target_link_libraries(OpenSCAD PRIVATE harfbuzz::harfbuzz)

  find_package(unofficial-fontconfig CONFIG REQUIRED)
  target_link_libraries(OpenSCAD PRIVATE unofficial::fontconfig::fontconfig)

  find_package(unofficial-glib CONFIG REQUIRED)
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${GLIB2_INCLUDE_DIRS})
  target_link_libraries(OpenSCAD PRIVATE unofficial::glib::gio unofficial::glib::glib unofficial::glib::gmodule unofficial::glib::gobject)

  find_package(double-conversion CONFIG REQUIRED)
  target_link_libraries(OpenSCAD PRIVATE double-conversion::double-conversion)

  find_library(GETTEXT_LIBRARY libintl)
  target_link_libraries(OpenSCAD PRIVATE ${GETTEXT_LIBRARY})

  # call before setting local CMAKE_MODULE_PATH so we use VCPKG version of FindGLEW
  find_graphics()

  # needed for QtQScintilla, maybe others
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
else()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
  if(MXECROSS)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/MXE")
  endif()

  if(NOT "$ENV{OPENSCAD_LIBRARIES}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{OPENSCAD_LIBRARIES}")
    if(APPLE)
      list(APPEND CMAKE_IGNORE_PREFIX_PATH "/opt/homebrew") # Homebrew Apple Silicon
      list(APPEND CMAKE_IGNORE_PREFIX_PATH "/usr/local")    # Homebrew Intel
    endif()
  endif()

  find_package(Eigen3 REQUIRED)
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})
  target_compile_definitions(OpenSCAD PRIVATE EIGEN_DONT_ALIGN)
  message(STATUS "Eigen: ${Eigen3_VERSION}")

  find_package(Boost 1.61 REQUIRED COMPONENTS system regex program_options)
  message(STATUS "Boost: ${Boost_VERSION}")
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(OpenSCAD PRIVATE ${Boost_LIBRARIES})
  target_compile_definitions(OpenSCAD PRIVATE BOOST_DLL_USE_STD_FS)

  find_package(HarfBuzz 0.9.19 REQUIRED QUIET)
  message(STATUS "Harfbuzz: ${HARFBUZZ_VERSION}")
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
  target_link_libraries(OpenSCAD PRIVATE ${HARFBUZZ_LIBRARIES})

  find_package(FontConfig 2.8.0 REQUIRED QUIET)
  message(STATUS "Fontconfig: ${FONTCONFIG_VERSION}")
  target_link_libraries(OpenSCAD PRIVATE ${FONTCONFIG_LIBRARIES})

  find_package(GLIB2 2.26 REQUIRED QUIET)
  message(STATUS "Glib: ${GLIB2_VERSION}")
  target_include_directories(OpenSCAD SYSTEM PRIVATE ${GLIB2_INCLUDE_DIRS})
  target_link_libraries(OpenSCAD PRIVATE ${GLIB2_LIBRARIES})

  find_package(DoubleConversion REQUIRED QUIET)
  target_link_libraries(OpenSCAD PRIVATE ${DoubleConversion_LIBRARIES})

  find_graphics()
endif()

if(USE_GLAD)
  target_link_libraries(OpenSCAD PRIVATE ${CMAKE_DL_LIBS})
endif()

if (("${Boost_VERSION}" VERSION_GREATER "1.72") AND ("${Boost_VERSION}" VERSION_LESS "1.76"))
  # Avoid warning messages from boost which are also caused by boost's code.
  #   https://github.com/boostorg/property_tree/issues/51
  target_compile_definitions(OpenSCAD PUBLIC BOOST_BIND_GLOBAL_PLACEHOLDERS)
endif()

if(ENABLE_CGAL)
  # Note: Saving CMAKE_MODULE_PATH as CGAL will overwrite it.
  # Reconsider this after CGAL 5.4: https://github.com/CGAL/cgal/pull/6029
  set(OPENSCAD_ORIGINAL_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
  set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)
  # Some older versions do not match with CGAL 5.0 REQUIRED, so we check after.
  find_package(CGAL REQUIRED COMPONENTS Core)
  if (${CGAL_MAJOR_VERSION} LESS 5)
    message(FATAL_ERROR "CGAL: ${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}.${CGAL_BUGFIX_VERSION} less than required minimum version 5.0")
  endif()
  message(STATUS "CGAL: ${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}.${CGAL_BUGFIX_VERSION}")
  target_compile_definitions(OpenSCAD PRIVATE ENABLE_CGAL)
  # The macro `CGAL_DEBUG` allows to force CGAL assertions, even if `NDEBUG` is defined,
  # Enabling CGAL assertions is necessary for us to catch them before they cause a crash
  # on bad geometry input.
  target_compile_definitions(OpenSCAD PRIVATE CGAL_DEBUG)
  target_compile_definitions(OpenSCAD PRIVATE CGAL_USE_GMPXX)
  if(TARGET CGAL::CGAL)
    target_link_libraries(OpenSCAD PRIVATE CGAL::CGAL CGAL::CGAL_Core)
    message(STATUS "CGAL: Using target CGAL::CGAL CGAL::CGAL_Core")
  else()
    target_link_libraries(OpenSCAD PRIVATE ${CGAL_LIBRARY} ${GMP_LIBRARIES} ${MPFR_LIBRARIES})
  endif()
  # revert any changes to module path from CGAL_Macros.cmake; see note above
  # Commented out code should work in CGAL>= 5.4
  #if (CGAL_MODULE_PATH_IS_SET)
  #  set(CMAKE_MODULE_PATH ${OPENSCAD_ORIGINAL_CMAKE_MODULE_PATH})
  #endif()
  set(CMAKE_MODULE_PATH ${OPENSCAD_ORIGINAL_CMAKE_MODULE_PATH})
endif(ENABLE_CGAL)

find_package(LibZip REQUIRED QUIET)
message(STATUS "libzip: ${LIBZIP_VERSION}")
target_include_directories(OpenSCAD SYSTEM PRIVATE ${LIBZIP_INCLUDE_DIR_ZIP} ${LIBZIP_INCLUDE_DIR_ZIPCONF})
target_link_libraries(OpenSCAD PRIVATE ${LIBZIP_LIBRARY})
target_compile_definitions(OpenSCAD PRIVATE ENABLE_LIBZIP)

find_package(Freetype 2.4.9 REQUIRED QUIET)
message(STATUS "Freetype: ${FREETYPE_VERSION_STRING}")
target_include_directories(OpenSCAD SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(OpenSCAD PRIVATE ${FREETYPE_LIBRARIES})

find_package(LibXml2 2.9 REQUIRED QUIET)
message(STATUS "LibXml2: ${LIBXML2_VERSION_STRING}")
target_include_directories(OpenSCAD SYSTEM PRIVATE ${LIBXML2_INCLUDE_DIR})
target_link_libraries(OpenSCAD PRIVATE ${LIBXML2_LIBRARIES})

if(ENABLE_PYTHON)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(CryptoPP REQUIRED)
  message(STATUS "Python enabled, using Crypto++ ${PC_CRYPTOPP_VERSION}")
  target_include_directories(OpenSCAD PRIVATE ${Python_INCLUDE_DIRS})
  target_include_directories(OpenSCAD PRIVATE ${CRYPTOPP_INCLUDE_DIRS})
  target_link_libraries(OpenSCAD PRIVATE ${Python_LIBRARIES})
  target_link_libraries(OpenSCAD PRIVATE ${CRYPTOPP_LIBRARIES})
endif()

if(ENABLE_HIDAPI)
  find_package(HidAPI 0.10 QUIET)
  if(HIDAPI_FOUND)
    set(INPUT_DRIVER_HIDAPI_SOURCES src/gui/input/HidApiInputDriver.cc)
    target_include_directories(OpenSCAD SYSTEM PRIVATE ${HIDAPI_INCLUDE_DIR})
    target_link_libraries(OpenSCAD PRIVATE ${HIDAPI_LIBRARY})
    target_compile_definitions(OpenSCAD PRIVATE ENABLE_HIDAPI)
    message(STATUS "HidAPI: ${HIDAPI_VERSION_STRING}")
  elseif(ALLOW_BUNDLED_HIDAPI)
    set(HIDAPI_SRC_DIR "src/ext/hidapi")
    file(STRINGS "${HIDAPI_SRC_DIR}/VERSION" HIDAPI_VERSION_STRING)
    target_include_directories(OpenSCAD SYSTEM PRIVATE "${HIDAPI_SRC_DIR}")
    set(INPUT_DRIVER_HIDAPI_SOURCES "${HIDAPI_SRC_DIR}/hid.c" src/gui/input/HidApiInputDriver.cc)
    target_compile_definitions(OpenSCAD PRIVATE ENABLE_HIDAPI)
    target_link_libraries(OpenSCAD PRIVATE setupapi hid)
    message(STATUS "HidAPI: ${HIDAPI_VERSION_STRING} (bundled)")
  else()
    message(STATUS "HIDAPI: disabled")
  endif()
else()
  message(STATUS "HIDAPI: disabled per user request")
endif()

if(ENABLE_SPNAV)
  find_package(SpNav QUIET)
  if(SPNAV_FOUND)
    message(STATUS "SpNav: found")
    set(INPUT_DRIVER_SPNAV_SOURCES src/gui/input/SpaceNavInputDriver.cc)
    target_include_directories(OpenSCAD SYSTEM PRIVATE ${SPNAV_INCLUDE_DIR})
    target_link_libraries(OpenSCAD PRIVATE ${SPNAV_LIBRARY})
    target_compile_definitions(OpenSCAD PRIVATE ENABLE_SPNAV)
  else()
    message(STATUS "SpNav: disabled")
  endif()
endif()

if(ENABLE_CAIRO)
  find_package(Cairo 1.14 QUIET)
  if (CAIRO_VERSION)
    message(STATUS "Cairo: ${CAIRO_VERSION}")
    target_include_directories(OpenSCAD SYSTEM PRIVATE ${CAIRO_INCLUDE_DIRS})
    target_link_libraries(OpenSCAD PRIVATE ${CAIRO_LIBRARIES})
    target_compile_definitions(OpenSCAD PRIVATE ENABLE_CAIRO)
  else()
    message(STATUS "Cairo: disabled")
  endif()
else()
  message(STATUS "Cairo: disabled per user request")
endif()

find_package(FLEX REQUIRED QUIET)
message(STATUS "Flex: ${FLEX_VERSION}")

find_package(BISON REQUIRED QUIET)
message(STATUS "Bison: ${BISON_VERSION}")

if(NOT MSVC)
  find_package(Lib3MF)
  if (Lib3MF_FOUND)
    message(STATUS "lib3mf: ${Lib3MF_VERSION}")
    target_link_libraries(OpenSCAD PRIVATE Lib3MF::Lib3MF)
    target_compile_definitions(OpenSCAD PRIVATE ENABLE_LIB3MF)
    if (Lib3MF_VERSION VERSION_GREATER_EQUAL 2)
      set(LIB3MF_SOURCES src/io/export_3mf_v2.cc src/io/import_3mf_v2.cc)
    else()
      set(LIB3MF_SOURCES src/io/export_3mf_v1.cc src/io/import_3mf_v1.cc)
    endif()
  else()
    set(LIB3MF_SOURCES src/io/export_3mf_dummy.cc src/io/import_3mf_dummy.cc)
    message(STATUS "lib3mf: disabled")
  endif()
endif()

# Automatically add the current source and build directories to the include path.
set(CMAKE_INCLUDE_CURRENT_DIR ON) # (does not propagate down to subdirectories)

if(USE_GLAD)
  target_compile_definitions(OpenSCAD PRIVATE USE_GLAD)
else()
  target_compile_definitions(OpenSCAD PRIVATE USE_GLEW)
endif()

target_include_directories(OpenSCAD PRIVATE
  "src"
  "src/ext"
  "src/ext/lexertl/include"
  "src/ext/libtess2/Include"

  # All the following should go away, not needed anymore after
  # PR #5312. Left here for now in case there are on-going PRs that
  # rely on these. Should be removed soon.
  "src/core"
  "src/core/customizer"
  "src/ext/json"
  "src/ext"
  "src/geometry"
  "src/geometry/cgal"
  "src/geometry/manifold"
  "src/glview"
  "src/glview/preview"
  "src/glview/cgal"
  "src/gui"
  "src/io"
  "src/platform"
  "src/utils"
)

FLEX_TARGET(openscad_lexer src/core/lexer.l ${OPENSCAD_LIB_OUTPUT_DIR}/lexer.cxx DEFINES_FILE ${OPENSCAD_LIB_OUTPUT_DIR}/lexer.hxx COMPILE_FLAGS ${WINCOMPAT})
BISON_TARGET(openscad_parser src/core/parser.y ${OPENSCAD_LIB_OUTPUT_DIR}/parser.cxx DEFINES_FILE ${OPENSCAD_LIB_OUTPUT_DIR}/parser.hxx COMPILE_FLAGS "-d -p parser")
ADD_FLEX_BISON_DEPENDENCY(openscad_lexer openscad_parser)

FLEX_TARGET(comment_lexer src/core/customizer/comment_lexer.l ${OPENSCAD_LIB_OUTPUT_DIR}/comment_lexer.cxx DEFINES_FILE ${OPENSCAD_LIB_OUTPUT_DIR}/comment_lexer.hxx COMPILE_FLAGS ${WINCOMPAT})
BISON_TARGET(comment_parser src/core/customizer/comment_parser.y ${OPENSCAD_LIB_OUTPUT_DIR}/comment_parser.cxx DEFINES_FILE ${OPENSCAD_LIB_OUTPUT_DIR}/comment_parser.hxx COMPILE_FLAGS "-d -p comment_parser")
ADD_FLEX_BISON_DEPENDENCY(comment_lexer comment_parser)

if(NOT HEADLESS)
  if (APPLE AND EXISTS /usr/local/opt/qt)
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt")
  endif()
  if (USE_QT6)
    if (APPLE AND EXISTS /usr/local/opt/qt@6)
      list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt@6")
    endif()
    if (APPLE AND EXISTS /opt/homebrew/opt/qt@6)
      list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@6")
    endif()
  else()
    if (APPLE AND EXISTS /usr/local/opt/qt@5)
      list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt@5")
    endif()
    if (APPLE AND EXISTS /opt/homebrew/opt/qt@5)
      list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@5")
    endif()
  endif()

  if("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.14")
    set_property(TARGET OpenSCAD PROPERTY AUTOGEN_ORIGIN_DEPENDS OFF)
  endif()
  set_property(TARGET OpenSCAD PROPERTY AUTOMOC ON)
  set_property(TARGET OpenSCAD PROPERTY AUTOUIC ON)
  set_property(TARGET OpenSCAD PROPERTY AUTORCC ON)
  set_property(TARGET OpenSCAD PROPERTY AUTOUIC_OPTIONS --tr q_)
  set_property(TARGET OpenSCAD PROPERTY AUTORCC_OPTIONS --threshold 10 --compress 9)
  if (USE_QT6)
    find_package(Qt6 COMPONENTS Core Core5Compat Widgets Multimedia OpenGL OpenGLWidgets Concurrent Network Svg REQUIRED QUIET)
    message(STATUS "Qt6: ${Qt6_VERSION}")

    if (Qt6_POSITION_INDEPENDENT_CODE)
      set_property(TARGET OpenSCAD PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()

    find_package(Qt6QScintilla 2.8.0 REQUIRED QUIET)
    message(STATUS "QScintilla: ${QT6QSCINTILLA_VERSION_STRING}")

    if(ENABLE_QTDBUS)
      find_package(Qt6DBus QUIET)
      if (Qt6DBus_FOUND)
        message(STATUS "DBus input driver enabled")
        target_compile_definitions(OpenSCAD PRIVATE ENABLE_DBUS)
        set(INPUT_DRIVER_DBUS_SOURCES src/gui/input/DBusInputDriver.cc)
        qt6_add_dbus_interface(INPUT_DRIVER_DBUS_SOURCES org.openscad.OpenSCAD.xml openscad_interface)
        qt6_add_dbus_adaptor(INPUT_DRIVER_DBUS_SOURCES org.openscad.OpenSCAD.xml gui/input/DBusInputDriver.h DBusInputDriver openscad_adaptor)
      else()
        message(STATUS "DBus input driver disabled as the QtDBus module could not be found.")
      endif()
    else()
      message(STATUS "DBus input driver disabled per user request.")
    endif()
  else()
    find_package(Qt5 5.12 COMPONENTS Core Widgets Multimedia OpenGL Concurrent Network Svg REQUIRED QUIET)
    message(STATUS "Qt5: ${Qt5_VERSION}")

    if (Qt5_POSITION_INDEPENDENT_CODE)
      set_property(TARGET OpenSCAD PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()

    if (("${Qt5_VERSION}" VERSION_GREATER_EQUAL "5.4"))
       target_compile_definitions(OpenSCAD PRIVATE USE_QOPENGLWIDGET)
    endif()

    find_package(Qt5QScintilla 2.8.0 REQUIRED QUIET)
    message(STATUS "QScintilla: ${QT5QSCINTILLA_VERSION_STRING}")

    if(ENABLE_QTDBUS)
      find_package(Qt5DBus QUIET)
      if (Qt5DBus_FOUND)
        message(STATUS "DBus input driver enabled")
        target_compile_definitions(OpenSCAD PRIVATE ENABLE_DBUS)
        set(INPUT_DRIVER_DBUS_SOURCES src/gui/input/DBusInputDriver.cc)
        qt5_add_dbus_interface(INPUT_DRIVER_DBUS_SOURCES org.openscad.OpenSCAD.xml openscad_interface)
        qt5_add_dbus_adaptor(INPUT_DRIVER_DBUS_SOURCES org.openscad.OpenSCAD.xml gui/input/DBusInputDriver.h DBusInputDriver openscad_adaptor)
      else()
        message(STATUS "DBus input driver disabled as the QtDBus module could not be found.")
      endif()
    else()
      message(STATUS "DBus input driver disabled per user request.")
    endif()

    if(ENABLE_GAMEPAD)
      find_package(Qt5Gamepad QUIET)
      if (Qt5Gamepad_FOUND)
        message(STATUS "Qt5Gamepad input driver enabled")
        set(GUI_SOURCES ${GUI_SOURCES} src/gui/input/QGamepadInputDriver.cc)
        target_compile_definitions(OpenSCAD PRIVATE ENABLE_QGAMEPAD)
      else()
        message(STATUS "Qt5Gamepad input driver disabled as the Qt5Gamepad module could not be found.")
      endif()
    else()
      message(STATUS "Qt5Gamepad input driver disabled per user request.")
    endif()
  endif()
endif()

# Setup ccache (if available) to speed up recompiles. It's especially useful
# when switching back and forth between branches where large numbers of files
# would otherwise need to be re-compiled each time.
if(USE_CCACHE)
  find_program(CCACHE_PATH ccache)
  if (CCACHE_PATH)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PATH})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PATH})
  endif()
endif()

target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_VERSION=${OPENSCAD_VERSION} OPENSCAD_SHORTVERSION=${OPENSCAD_SHORTVERSION} OPENSCAD_YEAR=${OPENSCAD_YEAR} OPENSCAD_MONTH=${OPENSCAD_MONTH})
if (DEFINED OPENSCAD_DAY)
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_DAY=${OPENSCAD_DAY})
endif()
if(DEFINED OPENSCAD_COMMIT)
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_COMMIT=${OPENSCAD_COMMIT})
endif()

#
# Platform specific settings
#

# Stack size 8MB; github issue 116
target_compile_definitions(OpenSCAD PRIVATE "STACKSIZE=${STACKSIZE}") # used as default in src/platform/PlatformUtils.h

if(NULLGL)
  set(OFFSCREEN_METHOD "NULLGL")
  message(STATUS "Offscreen OpenGL Context - using NULLGL")
else()
  SET(OFFSCREEN_METHOD "<undefined>")
endif()
if(APPLE)
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_OS="Mac OS X")
  message(STATUS "Offscreen OpenGL Context - using Apple CGL/NSOpenGL")
  set(PLATFORM_SOURCES src/platform/PlatformUtils-mac.mm src/io/imageutils-macosx.cc src/platform/CocoaUtils.mm)
  if(NOT HEADLESS)
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} src/gui/AppleEvents.cc)
  endif()
  if(NOT NULLGL)
    target_compile_definitions(OpenSCAD PRIVATE GL_SILENCE_DEPRECATION)
    set(OFFSCREEN_METHOD "Apple CGL/NSOpenGL")
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES}
        src/glview/offscreen-old/OffscreenContextNSOpenGL.mm
        src/glview/OffscreenContextCGL.cc)
  endif()
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(OpenSCAD PRIVATE ${COCOA_LIBRARY})
elseif(UNIX)
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_OS="Unix")
  set(PLATFORM_SOURCES src/io/imageutils-lodepng.cc src/platform/PlatformUtils-posix.cc)
  if(NOT NULLGL)
    set(OFFSCREEN_METHOD "Unix")
    if(ENABLE_EGL AND OpenGL_EGL_FOUND)
      target_compile_definitions(OpenSCAD PRIVATE ENABLE_EGL)
      set(OFFSCREEN_METHOD "${OFFSCREEN_METHOD} EGL")
      set(PLATFORM_SOURCES ${PLATFORM_SOURCES}
	src/glview/offscreen-old/OffscreenContextEGL.cc
        src/glview/OffscreenContextEGL.cc)
      if (NOT USE_GLAD)
        target_compile_definitions(OpenSCAD PRIVATE GLEW_EGL)
      endif()
      target_link_libraries(OpenSCAD PRIVATE OpenGL::EGL)
    endif()
    if(ENABLE_GLX AND OpenGL_GLX_FOUND)
      target_compile_definitions(OpenSCAD PRIVATE ENABLE_GLX)
      set(OFFSCREEN_METHOD "${OFFSCREEN_METHOD} GLX")
      set(PLATFORM_SOURCES ${PLATFORM_SOURCES}
          src/glview/offscreen-old/OffscreenContextGLX.cc
          src/glview/OffscreenContextGLX.cc)
      find_package(X11 REQUIRED)
      target_link_libraries(OpenSCAD PRIVATE X11::X11 ${CMAKE_DL_LIBS})
    endif()
    message(STATUS "Offscreen OpenGL Context - using ${OFFSCREEN_METHOD}")
  endif()
elseif(WIN32)
  target_compile_definitions(OpenSCAD PRIVATE NOGDI)
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_OS="Windows")
  message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
  set(PLATFORM_SOURCES src/io/imageutils-lodepng.cc src/platform/PlatformUtils-win.cc)
  if(NOT NULLGL)
    set(OFFSCREEN_METHOD "Windows WGL")
    message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} src/glview/offscreen-old/OffscreenContextWGL.cc)
  endif()
endif()

# NOTE: To keep HEADLESS builds working, do NOT add Qt-dependent sources here,
#       see GUI_SOURCES list below for that.
set(CORE_SOURCES
  src/Feature.cc
  src/FontCache.cc
  src/LibraryInfo.cc
  src/RenderStatistic.cc
  src/core/AST.cc
  src/core/Arguments.cc
  src/core/Assignment.cc
  src/core/BuiltinContext.cc
  src/core/Builtins.cc
  src/core/CSGNode.cc
  src/core/CSGTreeEvaluator.cc
  src/core/CgalAdvNode.cc
  src/core/Children.cc
  src/core/ColorNode.cc
  src/core/Context.cc
  src/core/ContextFrame.cc
  src/core/ContextMemoryManager.cc
  src/core/CsgOpNode.cc
  src/core/DrawingCallback.cc
  src/core/EvaluationSession.cc
  src/core/Expression.cc
  src/core/FreetypeRenderer.cc
  src/core/FunctionType.cc
  src/core/GroupModule.cc
  src/core/ImportNode.cc
  src/core/LinearExtrudeNode.cc
  src/core/LocalScope.cc
  src/core/ModuleInstantiation.cc
  src/core/NodeDumper.cc
  src/core/NodeVisitor.cc
  src/core/OffsetNode.cc
  src/core/Parameters.cc
  src/core/ProjectionNode.cc
  src/core/RenderNode.cc
  src/core/RenderVariables.cc
  src/core/RotateExtrudeNode.cc
  src/core/ScopeContext.cc
  src/core/SourceFile.cc
  src/core/SourceFileCache.cc
  src/core/StatCache.cc
  src/core/SurfaceNode.cc
  src/core/TextNode.cc
  src/core/TransformNode.cc
  src/core/Tree.cc
  src/core/UndefType.cc
  src/core/UserModule.cc
  src/core/Value.cc
  src/core/builtin_functions.cc
  src/core/control.cc
  src/core/customizer/Annotation.cc
  src/core/customizer/CommentParser.cc
  src/core/customizer/ParameterObject.cc
  src/core/customizer/ParameterSet.cc
  src/core/function.cc
  src/core/module.cc
  src/core/node.cc
  src/core/parsersettings.cc
  src/core/primitives.cc
  src/core/progress.cc
  src/ext/libtess2/Source/bucketalloc.c
  src/ext/libtess2/Source/dict.c
  src/ext/libtess2/Source/geom.c
  src/ext/libtess2/Source/mesh.c
  src/ext/libtess2/Source/priorityq.c
  src/ext/libtess2/Source/sweep.c
  src/ext/libtess2/Source/tess.c
  src/ext/lodepng/lodepng.cpp
  src/ext/polyclipping/clipper.cpp
  src/geometry/ClipperUtils.cc
  src/geometry/Geometry.cc
  src/geometry/GeometryCache.cc
  src/geometry/GeometryEvaluator.cc
  src/geometry/GeometryUtils.cc
  src/geometry/PolySet.cc
  src/geometry/PolySetBuilder.cc
  src/geometry/PolySetUtils.cc
  src/geometry/Polygon2d.cc
  src/geometry/boolean_utils.cc
  src/geometry/linalg.cc
  src/geometry/linear_extrude.cc
  src/glview/Camera.cc
  src/glview/ColorMap.cc
  src/glview/OffscreenContextFactory.cc
  src/glview/RenderSettings.cc
  src/glview/preview/CSGTreeNormalizer.cc
  src/handle_dep.cc
  src/io/DxfData.cc
  src/io/dxfdim.cc
  src/io/export.cc
  src/io/export_amf.cc
  src/io/export_dxf.cc
  src/io/export_obj.cc
  src/io/export_off.cc
  src/io/export_param.cc
  src/io/export_pdf.cc
  src/io/export_stl.cc
  src/io/export_svg.cc
  src/io/export_pov.cc
  src/io/export_param.cc
  src/io/export_wrl.cc
  src/io/fileutils.cc
  src/io/import_amf.cc
  src/io/import_json.cc
  src/io/import_obj.cc
  src/io/import_off.cc
  src/io/import_stl.cc
  src/io/import_svg.cc
  src/libsvg/circle.cc
  src/libsvg/data.cc
  src/libsvg/ellipse.cc
  src/libsvg/group.cc
  src/libsvg/libsvg.cc
  src/libsvg/line.cc
  src/libsvg/path.cc
  src/libsvg/polygon.cc
  src/libsvg/polyline.cc
  src/libsvg/rect.cc
  src/libsvg/shape.cc
  src/libsvg/svgpage.cc
  src/libsvg/text.cc
  src/libsvg/transformation.cc
  src/libsvg/tspan.cc
  src/libsvg/use.cc
  src/libsvg/util.cc
  src/platform/PlatformUtils.cc
  src/utils/StackCheck.h
  src/utils/calc.cc
  src/utils/degree_trig.cc
  src/utils/hash.cc
  src/utils/printutils.cc
  src/utils/svg.cc
  src/utils/version_check.h
  src/version.cc
  ${LIB3MF_SOURCES}
  ${PLATFORM_SOURCES}
  ${FLEX_openscad_lexer_OUTPUTS}
  ${BISON_openscad_parser_OUTPUTS}
  ${FLEX_comment_lexer_OUTPUTS}
  ${BISON_comment_parser_OUTPUTS})
if(ENABLE_PYTHON)
	list(APPEND CORE_SOURCES
		    src/core/pyopenscad.cc )
	target_compile_definitions(OpenSCAD PRIVATE ENABLE_PYTHON)
endif()

if(EXPERIMENTAL AND ENABLE_CGAL)
  list(APPEND CORE_SOURCES
  src/core/RoofNode.cc
  src/geometry/roof_ss.cc
  src/geometry/roof_vd.cc
)
endif()

set(CGAL_SOURCES
  src/geometry/cgal/cgalutils.cc
  src/geometry/cgal/cgalutils-applyops.cc
  src/geometry/cgal/cgalutils-applyops-hybrid.cc
  src/geometry/cgal/cgalutils-applyops-hybrid-minkowski.cc
  src/geometry/cgal/cgalutils-closed.cc
  src/geometry/cgal/cgalutils-convex.cc
  src/geometry/cgal/cgalutils-corefine.cc
  src/geometry/cgal/cgalutils-kernel.cc
  src/geometry/cgal/cgalutils-hybrid.cc
  src/geometry/cgal/cgalutils-mesh.cc
  src/geometry/cgal/cgalutils-minkowski.cc
  src/geometry/cgal/cgalutils-nef.cc
  src/geometry/cgal/cgalutils-orient.cc
  src/geometry/cgal/cgalutils-polyhedron.cc
  src/geometry/cgal/cgalutils-project.cc
  src/geometry/cgal/cgalutils-tess.cc
  src/geometry/cgal/cgalutils-triangulate.cc
  src/geometry/cgal/CGALHybridPolyhedron.cc
  src/geometry/cgal/CGAL_Nef_polyhedron.cc
  src/geometry/cgal/CGALCache.cc
  src/io/export_nef.cc
  src/io/import_nef.cc
  )

set(MANIFOLD_SOURCES
  src/geometry/manifold/ManifoldGeometry.cc
  src/geometry/manifold/manifoldutils.cc
  src/geometry/manifold/manifold-applyops.cc
  src/geometry/manifold/Polygon2d-manifold.cc
)

set(MANIFOLD_CGAL_SOURCES
  src/geometry/manifold/manifold-applyops-minkowski.cc
)

if(ENABLE_MANIFOLD)
  if(NOT DEFINED MANIFOLD_PAR)
    # Note: currently only Manifold-related code makes use of TBB parallelization
    # ("exact" CGAL numerics are not thread-safe)
    target_compile_options(OpenSCAD PRIVATE
      -DENABLE_TBB
    )
    find_package(TBB QUIET)
    if (NOT TBB_FOUND AND PKG_CONFIG_FOUND)
      pkg_check_modules(TBB tbb REQUIRED)
      add_library(TBB::tbb UNKNOWN IMPORTED)
      set_target_properties(TBB::tbb
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIRS}")
      set_target_properties(TBB::tbb
        PROPERTIES INTERFACE_LINK_LIBRARIES "${TBB_LINK_LIBRARIES}")
      list(GET TBB_LINK_LIBRARIES 0 TBB_IMPORTED_LOCATION)
      set_target_properties(TBB::tbb
        PROPERTIES IMPORTED_LOCATION "${TBB_IMPORTED_LOCATION}")
    endif()
    message(STATUS "TBB: ${TBB_VERSION}")
    target_link_libraries(OpenSCAD PRIVATE TBB::tbb)

    set(MANIFOLD_PAR ON CACHE STRING "Parallel backend" FORCE)
  endif()

  set(MANIFOLD_PYBIND OFF)
  set(MANIFOLD_TEST OFF)
  if(USE_BUILTIN_MANIFOLD)
    if(CMAKE_UNITY_BUILD)
      set(CMAKE_UNITY_BUILD OFF)
      add_subdirectory(submodules/manifold EXCLUDE_FROM_ALL)
      set(CMAKE_UNITY_BUILD ON)
    else()
      add_subdirectory(submodules/manifold EXCLUDE_FROM_ALL)
    endif()
    message(STATUS "mainfold submodule: ${manifold_VERSION}")
  else()
    find_package(manifold REQUIRED QUIET)
    message(STATUS "mainfold: ${manifold_VERSION}")
  endif()
  target_link_libraries(OpenSCAD PRIVATE manifold::manifold)
  add_sanitizers(manifold)

  if (USE_MANIFOLD_TRIANGULATOR)
    target_compile_definitions(OpenSCAD PRIVATE USE_MANIFOLD_TRIANGULATOR)
  endif()
endif()

#
# Offscreen OpenGL context source code
#
if(NULLGL)
  message(STATUS "NULLGL is set. Overriding OpenGL(TM) settings")
  set(OFFSCREEN_SOURCES
    src/glview/Renderer.cc
    src/glview/NULLGL.cc # contains several 'nullified' versions of above .cc files
    src/glview/OffscreenView.cc
    src/glview/OffscreenContextNULL.cc
    src/io/export_png.cc
    src/io/imageutils.cc)
else()
  if(USE_LEGACY_RENDERERS)
    set(LEGACY_RENDERER_SOURCES
      src/glview/cgal/LegacyCGALRenderer.cc
      src/glview/preview/LegacyOpenCSGRenderer.cc
      src/glview/preview/LegacyThrownTogetherRenderer.cc
      src/glview/LegacyRendererUtils.cc
    )
  endif()
  set(OFFSCREEN_SOURCES
    src/glview/OpenGLContext.cc
    src/glview/fbo.cc
    src/glview/Renderer.cc
    src/glview/system-gl.cc
    src/glview/VertexArray.cc
    src/glview/VertexState.cc
    src/glview/VBORenderer.cc
    src/glview/GLView.cc
    src/glview/hershey.cc
    src/glview/OffscreenView.cc
    src/glview/cgal/CGALRenderer.cc
    src/glview/cgal/CGALRenderUtils.cc
    src/glview/preview/OpenCSGRenderer.cc
    src/glview/preview/ThrownTogetherRenderer.cc
    src/io/export_png.cc
    src/io/imageutils.cc
    ${LEGACY_RENDERER_SOURCES}
    ${GLEW_SOURCES})
endif()


if(UNIX AND (NOT APPLE) AND (NOT HEADLESS))
  set(PLATFORM_INPUT_DRIVER_SOURCES src/gui/input/JoystickInputDriver.cc)
  target_compile_definitions(OpenSCAD PRIVATE ENABLE_JOYSTICK)
endif()

set(INPUT_DRIVER_SOURCES
  ${PLATFORM_INPUT_DRIVER_SOURCES}
  ${INPUT_DRIVER_HIDAPI_SOURCES}
  ${INPUT_DRIVER_SPNAV_SOURCES}
  ${INPUT_DRIVER_DBUS_SOURCES})

set(GUI_SOURCES
  ${GUI_SOURCES}
  src/openscad_gui.cc
  src/gui/AutoUpdater.cc
  src/gui/CGALWorker.cc
  src/gui/ViewportControl.cc
  src/gui/Console.cc
  src/gui/Dock.cc
  src/gui/Editor.cc
  src/gui/ErrorLog.cc
  src/gui/EventFilter.h
  src/gui/ExportPdfDialog.cc
  src/gui/FontListDialog.cc
  src/gui/FontListTableView.cc
  src/gui/InitConfigurator.cc
  src/gui/LaunchingScreen.cc
  src/gui/LibraryInfoDialog.cc
  src/gui/MainWindow.cc
  src/gui/Measurement.cc
  src/gui/Animate.cc
  src/gui/FontList.cc
  src/gui/MouseSelector.cc
  src/gui/OctoPrint.cc
  src/gui/OpenCSGWarningDialog.cc
  src/gui/OpenSCADApp.cc
  src/gui/Preferences.cc
  src/gui/PrintInitDialog.cc
  src/gui/PrintService.cc
  src/gui/ExternalToolInterface.cc
  src/gui/ProgressWidget.cc
  src/gui/QGLView.cc
  src/gui/QGLView2.cc
  src/gui/QSettingsCached.cc
  src/gui/QWordSearchField.cc
  src/gui/ScadApi.cc
  src/gui/ScadLexer.cc
  src/gui/ScintillaEditor.cc
  src/gui/Settings.cc
  src/gui/SettingsWriter.cc
  src/gui/TabManager.cc
  src/gui/TabWidget.cc
  src/gui/UIUtils.cc
  src/gui/WindowManager.cc
  src/gui/IgnoreWheelWhenNotFocused.cc
  src/gui/input/AxisConfigWidget.cc
  src/gui/input/ButtonConfigWidget.cc
  src/gui/input/InputDriver.cc
  src/gui/input/InputDriverManager.cc
  src/gui/input/InputEventMapper.cc
  src/gui/parameter/GroupWidget.cc
  src/gui/parameter/ParameterCheckBox.cc
  src/gui/parameter/ParameterComboBox.cc
  src/gui/parameter/ParameterSlider.cc
  src/gui/parameter/ParameterSpinBox.cc
  src/gui/parameter/ParameterText.cc
  src/gui/parameter/ParameterVector.cc
  src/gui/parameter/ParameterVirtualWidget.cc
  src/gui/parameter/ParameterWidget.cc
  ${INPUT_DRIVER_SOURCES}
  )

# header-only code
set(GUI_HEADER_ONLY
  src/gui/AboutDialog.h
  src/gui/Network.h
  src/gui/NetworkSignal.h
)

# To be added in Source for QtCreator to show them in project tree
set(GUI_HEADERS
    src/gui/Animate.h
    src/gui/AppleEvents.h
    src/gui/AutoUpdater.h
    src/gui/CGALWorker.h
    src/gui/Console.h
    src/gui/Dock.h
    src/gui/Editor.h
    src/gui/ErrorLog.h
    src/gui/EventFilter.h
    src/gui/ExportPdfDialog.h
    src/gui/FontList.h
    src/gui/FontListDialog.h
    src/gui/FontListTableView.h
    src/gui/IgnoreWheelWhenNotFocused.h
    src/gui/InitConfigurator.h
    src/gui/LaunchingScreen.h
    src/gui/LibraryInfoDialog.h
    src/gui/MainWindow.h
    src/gui/MouseSelector.h
    src/gui/Network.h
    src/gui/NetworkSignal.h
    src/gui/OctoPrint.h
    src/gui/OpenCSGWarningDialog.h
    src/gui/OpenSCADApp.h
    src/gui/Preferences.h
    src/gui/PrintInitDialog.h
    src/gui/PrintService.h
    src/gui/ProgressWidget.h
    src/gui/QGLView.h
    src/gui/QSettingsCached.h
    src/gui/QWordSearchField.h
    src/gui/ScadApi.h
    src/gui/ScadLexer.h
    src/gui/ScintillaEditor.h
    src/gui/Settings.h
    src/gui/SettingsWriter.h
    src/gui/TabManager.h
    src/gui/TabWidget.h
    src/gui/UIUtils.h
    src/gui/ViewportControl.h
    src/gui/WindowManager.h
    src/gui/qt-obsolete.h
    src/gui/qtgettext.h
    src/gui/AboutDialog.h
    src/gui/parameter/GroupWidget.h
    src/gui/parameter/ParameterCheckBox.h
    src/gui/parameter/ParameterComboBox.h
    src/gui/parameter/ParameterSlider.h
    src/gui/parameter/ParameterSpinBox.h
    src/gui/parameter/ParameterText.h
    src/gui/parameter/ParameterVector.h
    src/gui/parameter/ParameterVirtualWidget.h
    src/gui/parameter/ParameterWidget.h
)

# To be added in Source for QtCreator to show them in project tree
set(GUI_UIS
    ${GUI_UIS}
    src/gui/AboutDialog.ui
    src/gui/Animate.ui
    src/gui/Console.ui
    src/gui/ErrorLog.ui
    src/gui/ExportPdfDialog.ui
    src/gui/FontList.ui
    src/gui/FontListDialog.ui
    src/gui/LaunchingScreen.ui
    src/gui/LibraryInfoDialog.ui
    src/gui/MainWindow.ui
    src/gui/OpenCSGWarningDialog.ui
    src/gui/Preferences.ui
    src/gui/PrintInitDialog.ui
    src/gui/ProgressWidget.ui
    src/gui/ViewportControl.ui
    src/gui/input/AxisConfigWidget.ui
    src/gui/input/ButtonConfigWidget.ui
    src/gui/parameter/ParameterCheckBox.ui
    src/gui/parameter/ParameterComboBox.ui
    src/gui/parameter/ParameterDescriptionWidget.ui
    src/gui/parameter/ParameterSlider.ui
    src/gui/parameter/ParameterSpinBox.ui
    src/gui/parameter/ParameterText.ui
    src/gui/parameter/ParameterVector.ui
    src/gui/parameter/ParameterWidget.ui
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Ignore specific warning on external lib
  set_source_files_properties("src/ext/polyclipping/clipper.cpp" PROPERTIES COMPILE_FLAGS "-Wno-class-memaccess")
endif()

if (ENABLE_MANIFOLD)
  target_compile_definitions(OpenSCAD PRIVATE ENABLE_MANIFOLD)
  list(APPEND Sources ${MANIFOLD_SOURCES})
endif()
if (ENABLE_CGAL)
  list(APPEND Sources ${CGAL_SOURCES})
endif()
if (ENABLE_MANIFOLD AND ENABLE_CGAL)
  list(APPEND Sources ${MANIFOLD_CGAL_SOURCES})
endif()
list(APPEND Sources src/openscad.cc ${CORE_SOURCES} ${OFFSCREEN_SOURCES})

set_source_files_properties(
  # flex and bison tends to generate many macros, which does not work well with
  # unity build
  ${FLEX_openscad_lexer_OUTPUTS}
  ${BISON_openscad_parser_OUTPUTS}
  ${FLEX_comment_lexer_OUTPUTS}
  ${BISON_comment_parser_OUTPUTS}
  # manifold uses a lot of anonymous namespaces.
  # unity build will cause naming conflict for functions inside those namespaces.
  ${MANIFOLD_SOURCES}
  # files using opengl does not work well with unity build, again due to the
  # macros
  ${OFFSCREEN_SOURCES}
  ${PLATFORM_SOURCES}
  src/glview/OffscreenContextFactory.cc
  src/glview/RenderSettings.cc
  src/glview/Camera.cc
  src/glview/ColorMap.cc
  src/glview/preview/CSGTreeNormalizer.cc
  src/gui/QGLView.cc
  src/gui/QGLView2.cc
  # spnv.h references Xlib.h
  src/gui/input/SpaceNavInputDriver.cc
  # _USE_MATH_DEFINES should be defined before the first include of cmath,
  # unity build will violate this so we just exclude them
  src/core/BuiltinContext.cc
  src/geometry/roof_vd.cc
  src/geometry/manifold/manifold-applyops-minkowski.cc
  src/io/DxfData.cc
  src/utils/calc.cc
  src/utils/degree_trig.cc
  PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
if(HEADLESS)
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_NOGUI)
else()
  list(APPEND RESOURCE_FILES ${RESOURCE_DIR}/common.qrc)
  # GUI_UIS for .ui files to be indexed by Qt Creator despite AUTOUIC usage
  # GUI_HEADERS for .h files to be indexed by Qt Creator despite AUTOUIC usage
  list(APPEND Sources ${GUI_SOURCES} ${GUI_HEADER_ONLY} ${GUI_UIS} ${GUI_HEADERS})
endif()

if (SNAPSHOT)
  set(SNAPSHOT_SUFFIX "-nightly")
  set(SNAPSHOT_NAME_SUFFIX " (Nightly)")
  target_compile_definitions(OpenSCAD PRIVATE OPENSCAD_SNAPSHOT)
endif()
set(WINDOWS_RESOURCE_PATH ${CMAKE_BINARY_DIR}/openscad_win32${SNAPSHOT_SUFFIX}.rc)
set(MACOSX_BUNDLE_ICON_FILE icon${SNAPSHOT_SUFFIX}.icns)

if (APPLE)
  list(APPEND RESOURCE_FILES ${RESOURCE_DIR}/mac.qrc)
  list(APPEND RESOURCE_FILES ${RESOURCE_DIR}/icons/${MACOSX_BUNDLE_ICON_FILE})
elseif(WIN32)
  list(APPEND RESOURCE_FILES ${WINDOWS_RESOURCE_PATH})
endif()

if (SORT_BUILD)
  # Build the last modified sources first (to fail fast during development)
  execute_process(
    COMMAND ../scripts/sort_cmake_filelist.sh "${Sources}"
    OUTPUT_VARIABLE Sources)
endif()

target_sources(OpenSCAD PRIVATE ${Sources} ${RESOURCE_FILES})
find_program(SHELL_EXE NAMES sh bash $ENV{SHELL})
add_custom_command(TARGET OpenSCAD POST_BUILD
    COMMAND "${SHELL_EXE}"
    ARGS "${CMAKE_CURRENT_LIST_DIR}/scripts/translation-make.sh" "${SUFFIX_WITH_DASH}"
    COMMENT "Compiling language files")

if(APPLE AND NOT APPLE_UNIX)
  set_target_properties(OpenSCAD PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_ICON_FILE ${MACOSX_BUNDLE_ICON_FILE}
    MACOSX_BUNDLE_BUNDLE_VERSION ${OPENSCAD_YEAR}.${OPENSCAD_MONTH}.${OPENSCAD_DAY}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${OPENSCAD_YEAR}.${OPENSCAD_MONTH}
    RESOURCE "${RESOURCE_FILES}"
  )
  set(BUNDLE_RESOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/OpenSCAD.app/Contents/Resources)
  file(COPY ${CMAKE_SOURCE_DIR}/color-schemes DESTINATION ${BUNDLE_RESOURCES_DIR})
  file(COPY ${CMAKE_SOURCE_DIR}/examples DESTINATION ${BUNDLE_RESOURCES_DIR})
  file(COPY ${CMAKE_SOURCE_DIR}/fonts DESTINATION ${BUNDLE_RESOURCES_DIR})
  file(COPY ${CMAKE_SOURCE_DIR}/libraries DESTINATION ${BUNDLE_RESOURCES_DIR})
  file(COPY ${CMAKE_SOURCE_DIR}/locale DESTINATION ${BUNDLE_RESOURCES_DIR})
  file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${BUNDLE_RESOURCES_DIR})
  file(COPY ${CMAKE_SOURCE_DIR}/templates DESTINATION ${BUNDLE_RESOURCES_DIR})
elseif(MINGW)
  set_target_properties(OpenSCAD PROPERTIES
    LINK_FLAGS "-Wl,--stack,${STACKSIZE}"
  )
elseif(MSVC)
  set_target_properties(OpenSCAD PROPERTIES
    LINK_FLAGS "-subsystem:windows -ENTRY:mainCRTStartup -stack:${STACKSIZE}"
  )
endif()

if (USE_QT6)
  if(NOT HEADLESS)
    target_link_libraries(OpenSCAD PRIVATE
      Qt6::Core Qt6::Core5Compat Qt6::Widgets Qt6::Multimedia Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent Qt6::Network Qt6::Svg
      ${QT6QSCINTILLA_LIBRARY} ${Qt6DBus_LIBRARIES} ${Qt6Gamepad_LIBRARIES}
    )
  endif()
  if(MXECROSS)
    target_link_libraries(OpenSCAD PRIVATE Qt6::QSvgPlugin)
  endif()
else()
  if(NOT HEADLESS)
    target_link_libraries(OpenSCAD PRIVATE
      Qt5::Core Qt5::Widgets Qt5::Multimedia Qt5::OpenGL Qt5::Concurrent Qt5::Network Qt5::Svg
      ${QT5QSCINTILLA_LIBRARY} ${Qt5DBus_LIBRARIES} ${Qt5Gamepad_LIBRARIES}
    )
  endif()
  if(MXECROSS)
    target_link_libraries(OpenSCAD PRIVATE Qt5::QSvgPlugin)
  endif()
endif()

# Configure icon-related files, for release vs nightly
configure_file(${CMAKE_CURRENT_LIST_DIR}/openscad.appdata.xml.in ${CMAKE_CURRENT_LIST_DIR}/openscad.appdata.xml.in2)
configure_file(${RESOURCE_DIR}/icons/openscad.desktop.in ${RESOURCE_DIR}/icons/openscad.desktop)
configure_file(${RESOURCE_DIR}/common.qrc.in ${RESOURCE_DIR}/common.qrc)
configure_file(${RESOURCE_DIR}/mac.qrc.in ${RESOURCE_DIR}/mac.qrc)

# Installation
if(WIN32)
  set(OPENSCAD_BINDIR ".")
  set(OPENSCAD_INSTALL_RESOURCEDIR ".")
else()
  set(OPENSCAD_BINDIR ${CMAKE_INSTALL_BINDIR})
  set(OPENSCAD_INSTALL_RESOURCEDIR ${CMAKE_INSTALL_DATAROOTDIR}/openscad${SUFFIX_WITH_DASH})
endif()

if(NOT APPLE OR APPLE_UNIX)
  set_target_properties(OpenSCAD PROPERTIES OUTPUT_NAME openscad${SUFFIX_WITH_DASH})
  install(TARGETS OpenSCAD RUNTIME DESTINATION "${OPENSCAD_BINDIR}")
  if(WIN32)
    if(USE_MIMALLOC AND MI_LINK_SHARED)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        install(FILES ${CMAKE_SOURCE_DIR}/submodules/mimalloc/bin/mimalloc-redirect.dll DESTINATION ".")
      elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        install(FILES ${CMAKE_SOURCE_DIR}/submodules/mimalloc/bin/mimalloc-redirect32.dll DESTINATION ".")
      endif()
      install(FILES ${CMAKE_BINARY_DIR}/submodules/mimalloc/mimalloc.dll DESTINATION ".")
    endif()
  else()
    install(FILES ${CMAKE_CURRENT_LIST_DIR}/doc/openscad.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 RENAME openscad${SUFFIX_WITH_DASH}.1)
    install(FILES ${CMAKE_CURRENT_LIST_DIR}/openscad.appdata.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo RENAME org.openscad.OpenSCAD${SUFFIX_WITH_DASH}.appdata.xml)
    install(FILES ${RESOURCE_DIR}/icons/openscad.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications RENAME openscad${SUFFIX_WITH_DASH}.desktop)
    install(FILES ${RESOURCE_DIR}/icons/openscad.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages RENAME openscad${SUFFIX_WITH_DASH}.xml)
    install(FILES ${RESOURCE_DIR}/icons/openscad${SNAPSHOT_SUFFIX}-48.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps RENAME openscad${SNAPSHOT_SUFFIX}.png)
    install(FILES ${RESOURCE_DIR}/icons/openscad${SNAPSHOT_SUFFIX}-64.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps RENAME openscad${SNAPSHOT_SUFFIX}.png)
    install(FILES ${RESOURCE_DIR}/icons/openscad${SNAPSHOT_SUFFIX}-128.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps RENAME openscad${SNAPSHOT_SUFFIX}.png)
    install(FILES ${RESOURCE_DIR}/icons/openscad${SNAPSHOT_SUFFIX}-256.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps RENAME openscad${SNAPSHOT_SUFFIX}.png)
    install(FILES ${RESOURCE_DIR}/icons/openscad${SNAPSHOT_SUFFIX}-512.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps RENAME openscad${SNAPSHOT_SUFFIX}.png)
  endif()
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/color-schemes DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}")
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}")
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/fonts DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}" PATTERN ".uuid" EXCLUDE)
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/libraries DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}" PATTERN ".git*" EXCLUDE)
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/locale DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}" FILES_MATCHING PATTERN "*/LC_MESSAGES/openscad.mo")
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/shaders DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}")
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/templates DESTINATION "${OPENSCAD_INSTALL_RESOURCEDIR}" FILES_MATCHING PATTERN "*.json")
else()
  install(TARGETS ${CMAKE_PROJECT_NAME}
      BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# Packaging: CPACK_* settings should be configured before `include(CPack)`
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "OpenSCAD")
set(CPACK_PACKAGE_VENDOR "The OpenSCAD Developers")
set(CPACK_PACKAGE_VERSION "${OPENSCAD_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Programmer's Solid 3D CAD Modeler")

if(MXECROSS)
  set(CPACK_GENERATOR ZIP;NSIS)
  set(CPACK_SYSTEM_NAME ${PACKAGE_ARCH})
  set(CPACK_PACKAGE_EXECUTABLES "openscad;${CPACK_PACKAGE_NAME}${SNAPSHOT_NAME_SUFFIX}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}${SNAPSHOT_NAME_SUFFIX}")
  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}${SNAPSHOT_NAME_SUFFIX}")
  if("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.22")
    set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
  else()
    message(FATAL_ERROR "CPACK_NSIS_IGNORE_LICENSE_PAGE requires cmake 3.22")
  endif()
  set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "\
    !include \\\"FileFunc.nsh\\\"\n\
    !include \\\"${CMAKE_SOURCE_DIR}/cmake/nsis/mingw-file-association.nsh\\\"\
  ")
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "\
    !include \\\"${CMAKE_BINARY_DIR}/nsis-extra-install-commands.nsh\\\"\
  ")
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "\
    !include \\\"${CMAKE_BINARY_DIR}/nsis-extra-uninstall-commands.nsh\\\"\
  ")
endif()
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
# None of config time variables are available for CPACK_PROJECT_CONFIG_FILE, so we configure it now with values.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/openscad_cpack.cmake.in"
               "${CMAKE_BINARY_DIR}/openscad_cpack.cmake" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/extra-install-commands.nsh.in"
               "${CMAKE_BINARY_DIR}/nsis-extra-install-commands.nsh" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/extra-uninstall-commands.nsh.in"
               "${CMAKE_BINARY_DIR}/nsis-extra-uninstall-commands.nsh" @ONLY)
# CPACK_PROJECT_CONFIG_FILE is for configuring CPack-generator specific settings
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/openscad_cpack.cmake")
set(CPACK_THREADS 0)

if(ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(OFFLINE_DOCS)
  add_subdirectory(resources)
endif()

configure_file(${RESOURCE_DIR}/openscad_win32${SNAPSHOT_SUFFIX}.rc.in
	       ${CMAKE_BINARY_DIR}/openscad_win32${SNAPSHOT_SUFFIX}.rc)
include(CPack)

add_sanitizers(OpenSCAD)

if(INFO)
  include("cmake/Modules/info.cmake")
endif()

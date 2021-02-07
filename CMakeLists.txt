#
# CMake build system for OpenSCAD
#
# Configuration variables
#   -DHEADLESS=<ON|OFF>
#   -DNULLGL=<ON|OFF>
#   -DSNAPSHOT=<ON|OFF>
#   -DEXPERIMENTAL=<ON|OFF>
#
#  TODO
#   find packages for spnav, hidapi


if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
endif()
cmake_minimum_required(VERSION 3.3)

if(POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD)
endif()

project(openscad)

option(INFO "Display build configuration info at end of cmake config" ON)
option(EXPERIMENTAL "Enable Experimental Features" OFF)
option(SNAPSHOT "Create dev snapshot, uses nightly icons" OFF)
option(HEADLESS "Build without GUI frontend" OFF)
option(NULLGL "Build without OpenGL, (implies HEADLESS=ON) " OFF)
option(IDPREFIX "Prefix CSG nodes with index # (debugging purposes only, will break node cache)" OFF)
option(PROFILE "Enable compiling with profiling / test coverage instrumentation" OFF)

if(PROFILE)
  SET(GCC_COVERAGE_COMPILE_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage -fprofile-dir=.gcov")
  SET(GCC_COVERAGE_LINK_FLAGS    "--coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
  # Make sure to override default optimization for Release type
  string(REPLACE "-O3" "-O0" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else()
  # Use same optimization as qmake build for Release
  string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

if (NULLGL)
  set(HEADLESS ON)
endif()
if(EXPERIMENTAL)
  add_definitions(-DENABLE_EXPERIMENTAL)
endif()
if(IDPREFIX)
  add_definitions(-DIDPREFIX)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/objects")
set(AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/objects")
file(MAKE_DIRECTORY ${AUTOGEN_BUILD_DIR})

# Default to Release build
#if(NOT CMAKE_BUILD_TYPE)
#  message(STATUS "CMAKE_BUILD_TYPE not specified.  Defaulting to 'Release'")
#  message(STATUS "Usage: cmake -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel] .")
#  set(CMAKE_BUILD_TYPE Release)
#else()
#  message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
#endif()

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

set(CMAKE_CXX_STANDARD 14)
add_definitions(-D_REENTRANT -DUNICODE -D_UNICODE)
# Stack size 8MB; github issue 116
add_definitions(-DSTACKSIZE=8388608)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frounding-math")
  if (WIN32) # gcc bug spams warnings, See issue #2771
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
  endif()
endif()

set(BOOST_DIRECTLY_REQUIRED_LIBRARIES filesystem system regex program_options)

find_package(PkgConfig)

macro(find_graphics)
  # NULLGL - Allow us to build without OpenGL(TM). run 'cmake .. -DNULLGL=1'
  # Most tests will fail, but it can be used for testing/experiments
  if(NULLGL)
    add_definitions(-DNULLGL)
  else()
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenGL REQUIRED QUIET)
    list(APPEND COMMON_LIBRARIES ${OPENGL_LIBRARIES})

    if(MSVC)
      find_package(GLEW CONFIG REQUIRED QUIET)
      message(STATUS "GLEW: Using target GLEW::GLEW")
      list(APPEND COMMON_LIBRARIES GLEW::GLEW)

      find_path(OPENCSG_INCLUDE_DIRS opencsg/opencsg.h)
      include_directories("${OPENCSG_INCLUDE_DIRS}/opencsg")
    else()
      find_package(GLEW REQUIRED)
      message(STATUS "GLEW_INCLUDE_DIR: ${GLEW_INCLUDE_DIR}")
      message(STATUS "GLEW_LIBRARY: ${GLEW_LIBRARY}")
      include_directories("${GLEW_INCLUDE_DIR}")
      list(APPEND COMMON_LIBRARIES ${GLEW_LIBRARY})

      find_path(OPENCSG_INCLUDE_DIRS opencsg.h)
      include_directories("${OPENCSG_INCLUDE_DIRS}")
    endif()
    find_library(OPENCSG_LIBRARY opencsg)
    list(APPEND COMMON_LIBRARIES ${OPENCSG_LIBRARY})

    message(STATUS "OpenCSG: ${OPENCSG_VERSION_STRING}")
    add_definitions(-DENABLE_OPENCSG)
  endif()
endmacro(find_graphics)

# NOTE FindCGAL.cmake resets the BOOST_* and Boost_* variables,
#   because it looks for Boost itself, so it has to be called
#   before we look for our own required boost libraries.
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)
find_package(CGAL REQUIRED)
message(STATUS "CGAL: ${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}")
add_definitions(-DENABLE_CGAL)
if(TARGET CGAL::CGAL)
  list(APPEND COMMON_LIBRARIES CGAL::CGAL)
  message(STATUS "CGAL: Using target CGAL::CGAL")
else()
  list(APPEND COMMON_LIBRARIES ${CGAL_LIBRARY})
  list(APPEND COMMON_LIBRARIES ${GMP_LIBRARIES})
  list(APPEND COMMON_LIBRARIES ${MPFR_LIBRARIES})
endif()
# revert any changes to module path from CGAL_Macros.cmake
if (CGAL_MODULE_PATH_IS_SET)
  set(CMAKE_MODULE_PATH ORIGINAL_CMAKE_MODULE_PATH)
endif()


if (MSVC)
  # Flex lexer options
  set(WINCOMPAT "--wincompat --nounistd")
  add_definitions(-D_USE_MATH_DEFINES)

  message("COMMON_LIBRARIES ${COMMON_LIBRARIES}")
  find_package(Eigen3 CONFIG REQUIRED)
  list(APPEND COMMON_LIBRARIES Eigen3::Eigen)

  set(Boost_USE_STATIC_LIBS TRUE)
  find_package(Boost 1.36 REQUIRED COMPONENTS ${BOOST_DIRECTLY_REQUIRED_LIBRARIES})
  message(STATUS "Boost: ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
  message("BOOST DIRS ${Boost_INCLUDE_DIRS}")
  include_directories(${Boost_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES ${Boost_LIBRARIES})

  find_package(harfbuzz CONFIG REQUIRED)
  find_path(HARFBUZZ_INCLUDE_DIRS harfbuzz)
  include_directories(${HARFBUZZ_INCLUDE_DIRS}/harfbuzz)
  list(APPEND COMMON_LIBRARIES harfbuzz::harfbuzz)

  find_package(unofficial-fontconfig CONFIG REQUIRED)
  list(APPEND COMMON_LIBRARIES unofficial::fontconfig::fontconfig)

  find_package(unofficial-glib CONFIG REQUIRED)
  include_directories(${GLIB2_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES unofficial::glib::gio unofficial::glib::glib unofficial::glib::gmodule unofficial::glib::gobject)

  find_package(double-conversion CONFIG REQUIRED)
  list(APPEND COMMON_LIBRARIES double-conversion::double-conversion)

  find_library(GETTEXT_LIBRARY libintl)
  list(APPEND COMMON_LIBRARIES ${GETTEXT_LIBRARY})

  # call before setting local CMAKE_MODULE_PATH so we use VCPKG version of FindGLEW
  find_graphics()

  # needed for Qt5QScintilla, maybe others
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

else()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
  set(CMAKE_PREFIX_PATH "$ENV{OPENSCAD_LIBRARIES}")

  find_package(Eigen3 QUIET)
  if (NOT Eigen3_FOUND)
    # Ubuntu Trusty Eigen package installs FindEigen3.cmake but assumes cmake-2.8, not cmake3
    # See https://packages.ubuntu.com/trusty/all/libeigen3-dev/filelist
    list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake-2.8/Modules")
    find_package(Eigen3 REQUIRED QUIET)
    list(REMOVE_ITEM CMAKE_MODULE_PATH "/usr/share/cmake-2.8/Modules")
  endif()
  if (EIGEN3_VERSION)
    message(STATUS "Eigen3: ${EIGEN3_VERSION}")
  else()
    message(STATUS "Eigen3: ${EIGEN3_VERSION_STRING}")
  endif()
  include_directories(${EIGEN3_INCLUDE_DIR})
  add_definitions(-DEIGEN_DONT_ALIGN)

  find_package(Boost 1.36 REQUIRED COMPONENTS ${BOOST_DIRECTLY_REQUIRED_LIBRARIES})
  message(STATUS "Boost: ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
  include_directories(${Boost_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES ${Boost_LIBRARIES})

  find_package(HarfBuzz 0.9.19 REQUIRED QUIET)
  message(STATUS "Harfbuzz: ${HARFBUZZ_VERSION}")
  include_directories(${HARFBUZZ_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES ${HARFBUZZ_LIBRARIES})

  find_package(FontConfig 2.8.0 REQUIRED QUIET)
  message(STATUS "Fontconfig: ${FONTCONFIG_VERSION}")
  list(APPEND COMMON_LIBRARIES ${FONTCONFIG_LIBRARIES})

  find_package(GLIB2 2.26 REQUIRED QUIET)
  message(STATUS "Glib: ${GLIB2_VERSION}")
  include_directories(${GLIB2_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES ${GLIB2_LIBRARIES})

  list(APPEND COMMON_LIBRARIES double-conversion)

  find_graphics()
endif()

find_package(LibZip REQUIRED QUIET)
include_directories(${LIBZIP_INCLUDE_DIR_ZIP})
include_directories(${LIBZIP_INCLUDE_DIR_ZIPCONF})
list(APPEND COMMON_LIBRARIES ${LIBZIP_LIBRARY})
add_definitions(-DENABLE_LIBZIP)

find_package(Freetype 2.4.9 REQUIRED QUIET)
message(STATUS "Freetype: ${FREETYPE_VERSION_STRING}")
include_directories(${FREETYPE_INCLUDE_DIRS})
list(APPEND COMMON_LIBRARIES ${FREETYPE_LIBRARIES})

find_package(LibXml2 2.9 REQUIRED QUIET)
message(STATUS "LibXml2: ${LIBXML2_VERSION_STRING}")
include_directories(${LIBXML2_INCLUDE_DIR})
list(APPEND COMMON_LIBRARIES ${LIBXML2_LIBRARIES})

find_package(Cairo 1.14 QUIET)
if (CAIRO_VERSION)
  message(STATUS "Cairo: ${CAIRO_VERSION}")
  include_directories(${CAIRO_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES ${CAIRO_LIBRARIES})
  add_definitions(-DENABLE_CAIRO)
else()
  message(STATUS "Cairo: disabled")
endif()

find_package(FLEX REQUIRED QUIET)
message(STATUS "Flex: ${FLEX_VERSION}")

find_package(BISON REQUIRED QUIET)
message(STATUS "Bison: ${BISON_VERSION}")

if(NOT MSVC)
  find_package(Lib3MF REQUIRED QUIET)
  add_definitions(${LIB3MF_CFLAGS})
  include_directories(${LIB3MF_INCLUDE_DIRS})
  list(APPEND COMMON_LIBRARIES ${LIB3MF_LIBRARIES})
endif()

# Output compilation database (compile_commands.json), so we can e.g. run clang-tidy or other tools separately
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
# Use clang-tidy if run with -DCLANG_TIDY=1
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
set(CLANG_TIDY ${CLANG_TIDY} CACHE BOOL "Enable clang-tidy")
if(CLANG_TIDY AND CLANG_TIDY_EXE)
  include(RegexUtils)
  escape_string_as_regex(regex "${CMAKE_SOURCE_DIR}/src")
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-header-filter=${regex}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(src)

FLEX_TARGET(openscad_lexer src/lexer.l ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lexer.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lexer.hxx COMPILE_FLAGS ${WINCOMPAT})
BISON_TARGET(openscad_parser src/parser.y ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/parser.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/parser.hxx COMPILE_FLAGS "-d -p parser")
ADD_FLEX_BISON_DEPENDENCY(openscad_lexer openscad_parser)

FLEX_TARGET(comment_lexer src/comment_lexer.l ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_lexer.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_lexer.hxx COMPILE_FLAGS ${WINCOMPAT})
BISON_TARGET(comment_parser src/comment_parser.y ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_parser.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_parser.hxx COMPILE_FLAGS "-d -p comment_parser")
ADD_FLEX_BISON_DEPENDENCY(comment_lexer comment_parser)

if(NOT HEADLESS)
  if (APPLE AND EXISTS /usr/local/opt/qt)
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt")
  endif()

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTORCC ON)
  find_package(Qt5 COMPONENTS Core Widgets Multimedia OpenGL Concurrent Network REQUIRED QUIET)
  message(STATUS "Qt5: ${Qt5_VERSION}")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()

  if (("${Qt5_VERSION}" VERSION_EQUAL "5.4") OR ("${Qt5_VERSION}" VERSION_GREATER "5.4"))
    add_definitions(-DUSE_QOPENGLWIDGET)
  endif()

  find_package(Qt5QScintilla 2.8.0 REQUIRED QUIET)
  message(STATUS "QScintilla: ${QT5QSCINTILLA_VERSION_STRING}")
  add_definitions(-DUSE_SCINTILLA_EDITOR)
  add_definitions(-DENABLE_MDI)

  find_package(Qt5DBus QUIET)
  if (Qt5DBus_FOUND)
    message(STATUS "DBus input driver enabled")
    add_definitions(-DENABLE_DBUS)
    set(INPUT_DRIVER_DBUS_SOURCES src/input/DBusInputDriver.cc)
    qt5_add_dbus_interface(INPUT_DRIVER_DBUS_SOURCES org.openscad.OpenSCAD.xml openscad_interface)
    qt5_add_dbus_adaptor(INPUT_DRIVER_DBUS_SOURCES org.openscad.OpenSCAD.xml input/DBusInputDriver.h DBusInputDriver openscad_adaptor)
  else()
    message(STATUS "DBus input driver disabled as the QtDBus module could not be found.")
  endif()

  find_package(Qt5Gamepad QUIET)
  if (Qt5Gamepad_FOUND)
    message(STATUS "Gamepad input driver enabled")
    set(GUI_SOURCES ${GUI_SOURCES} src/input/QGamepadInputDriver.cc)
    add_definitions(-DENABLE_QGAMEPAD)
  else()
    message(STATUS "Gamepad input driver disabled as the Qt5Gamepad module could not be found.")
  endif()

endif()


# Setup ccache (if available) to speed up recompiles. It's especially useful
# when switching back and forth between branches where large numbers of files
# would otherwise need to be re-compiled each time.
find_program(CCACHE_PATH ccache)
if (CCACHE_PATH)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PATH})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PATH})
endif()

#
# Version
#
string(TIMESTAMP VERSION "%Y.%m.%d")
string(REPLACE "-" ";" SPLITVERSION ${VERSION})
list(GET SPLITVERSION 0 OPENSCAD_SHORTVERSION)
string(REGEX MATCHALL "^[0-9]+|[0-9]+|[0-9]+$" VERSIONLIST "${OPENSCAD_SHORTVERSION}")
list(GET VERSIONLIST 0 OPENSCAD_YEAR)
list(GET VERSIONLIST 1 OPENSCAD_MONTH)
math(EXPR OPENSCAD_MONTH ${OPENSCAD_MONTH}) # get rid of leading zero
list(LENGTH VERSIONLIST VERSIONLEN)
if (${VERSIONLEN} EQUAL 3)
  list(GET VERSIONLIST 2 OPENSCAD_DAY)
  math(EXPR OPENSCAD_DAY ${OPENSCAD_DAY}) # get rid of leading zero
endif()

add_definitions(-DOPENSCAD_VERSION=${VERSION} -DOPENSCAD_SHORTVERSION=${OPENSCAD_SHORTVERSION} -DOPENSCAD_YEAR=${OPENSCAD_YEAR} -DOPENSCAD_MONTH=${OPENSCAD_MONTH})
if (DEFINED OPENSCAD_DAY)
  add_definitions(-DOPENSCAD_DAY=${OPENSCAD_DAY})
endif()
if(DEFINED OPENSCAD_COMMIT)
  add_definitions(-DOPENSCAD_COMMIT=${OPENSCAD_COMMIT})
endif()

#
# Platform specific settings
#
if(APPLE)
  message(STATUS "Offscreen OpenGL Context - using Apple CGL")
  set(PLATFORM_SOURCES src/PlatformUtils-mac.mm src/imageutils-macosx.cc src/CocoaUtils.mm)
  if(NOT HEADLESS)
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} src/AppleEvents.cc)
  endif()
  if(NULLGL)
    add_definitions(-DOPENSCAD_OS="Mac OS X")
  else()
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} src/OffscreenContextCGL.mm)
  endif()
  find_library(COCOA_LIBRARY Cocoa)
  set(PLATFORM_LIBS ${COCOA_LIBRARY})
elseif(UNIX)
  message(STATUS "Offscreen OpenGL Context - using Unix GLX on X11")
  set(PLATFORM_SOURCES src/imageutils-lodepng.cc src/PlatformUtils-posix.cc)
  if(NULLGL)
    add_definitions(-DOPENSCAD_OS="Unix")
  else()
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} src/OffscreenContextGLX.cc)
    find_library(X11_LIBRARY X11)
    set(PLATFORM_LIBS ${X11_LIBRARY})
  endif()
elseif(WIN32)
  add_definitions(-DNOGDI)
  message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
  set(PLATFORM_SOURCES src/imageutils-lodepng.cc src/PlatformUtils-win.cc)
  if(NULLGL)
    add_definitions(-DOPENSCAD_OS="Windows")
  else()
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES} src/OffscreenContextWGL.cc)
  endif()
endif()

include_directories("src/ext/libtess2/Include")

# NOTE: To keep HEADLESS builds working, do NOT add Qt-dependent sources here,
#       see GUI_SOURCES list below for that.
set(CORE_SOURCES
  src/annotation.cc
  src/boost-utils.cc
  src/builtin.cc
  src/builtincontext.cc
  src/calc.cc
  src/cgaladv.cc
  src/clipper-utils.cc
  src/color.cc
  src/colormap.cc
  src/context.cc
  src/control.cc
  src/comment.cpp
  src/csgnode.cc
  src/csgops.cc
  src/degree_trig.cc
  src/dxfdata.cc
  src/dxfdim.cc
  src/evalcontext.cc
  src/export.cc
  src/export_3mf.cc
  src/export_amf.cc
  src/export_dxf.cc
  src/export_off.cc
  src/export_pdf.cc
  src/export_stl.cc
  src/export_svg.cc
  src/expr.cc
  src/feature.cc
  src/fileutils.cc
  src/func.cc
  src/function.cc
  src/handle_dep.cc
  src/hash.cc
  src/import.cc
  src/import_3mf.cc
  src/import_amf.cc
  src/import_stl.cc
  src/import_off.cc
  src/import_svg.cc
  src/linalg.cc
  src/linearextrude.cc
  src/localscope.cc
  src/modcontext.cc
  src/module.cc
  src/node.cc
  src/nodedumper.cc
  src/offset.cc
  src/parsersettings.cc
  src/polyset.cc
  src/polyset-utils.cc
  src/primitives.cc
  src/printutils.cc
  src/progress.cc
  src/projection.cc
  src/render.cc
  src/rendersettings.cc
  src/rotateextrude.cc
  src/stackcheck.h
  src/surface.cc
  src/svg.cc
  src/text.cc
  src/transform.cc
  src/value.cc
  src/version.cc
  src/Assignment.cc
  src/AST.cc
  src/Camera.cc
  src/CSGTreeNormalizer.cc
  src/DrawingCallback.cc
  src/FileModule.cc
  src/FontCache.cc
  src/FreetypeRenderer.cc
  src/Geometry.cc
  src/GeometryCache.cc
  src/GeometryUtils.cc
  src/GroupModule.cc
  src/LibraryInfo.cc
  src/ModuleInstantiation.cc
  src/ModuleCache.cc
  src/NodeVisitor.cc
  src/PlatformUtils.cc
  src/Polygon2d.cc
  src/RenderStatistic.cc
  src/StatCache.cc
  src/UserModule.cc
  src/Tree.cc
  src/ext/lodepng/lodepng.cpp
  src/ext/polyclipping/clipper.cpp
  src/ext/libtess2/Source/bucketalloc.c
  src/ext/libtess2/Source/dict.c
  src/ext/libtess2/Source/geom.c
  src/ext/libtess2/Source/mesh.c
  src/ext/libtess2/Source/priorityq.c
  src/ext/libtess2/Source/sweep.c
  src/ext/libtess2/Source/tess.c
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
  src/libsvg/util.cc
  src/parameter/parameterset.cpp
  ${PLATFORM_SOURCES}
  ${FLEX_openscad_lexer_OUTPUTS}
  ${BISON_openscad_parser_OUTPUTS}
  ${FLEX_comment_lexer_OUTPUTS}
  ${BISON_comment_parser_OUTPUTS})

set(CGAL_SOURCES
  src/cgalutils.cc
  src/cgalutils-applyops.cc
  src/cgalutils-polyhedron.cc
  src/cgalutils-project.cc
  src/cgalutils-tess.cc
  src/export_nef.cc
  src/import_nef.cc
  src/CGAL_Nef_polyhedron.cc
  src/CGALCache.cc
  src/CSGTreeEvaluator.cc
  src/Polygon2d-CGAL.cc
  src/GeometryEvaluator.cc)

#
# Offscreen OpenGL context source code
#
if(NULLGL)
  message(STATUS "NULLGL is set. Overriding OpenGL(TM) settings")
  set(OFFSCREEN_SOURCES
    src/export_png.cc
    src/imageutils.cc
    src/renderer.cc
    src/render.cc
    src/NULLGL.cc # contains several 'nullified' versions of above .cc files
    src/OffscreenView.cc
    src/OffscreenContextNULL.cc
    src/${OFFSCREEN_IMGUTILS_SOURCE})
else()
  set(OFFSCREEN_SOURCES
    src/export_png.cc
    src/fbo.cc
    src/imageutils.cc
    src/render.cc
    src/renderer.cc
    src/system-gl.cc
    src/VertexArray.cc
    src/VBORenderer.cc
    src/CGALRenderer.cc
    src/GLView.cc
    src/OffscreenView.cc
    src/OpenCSGRenderer.cc
    src/ThrownTogetherRenderer.cc)
endif()


if(UNIX AND (NOT APPLE) AND (NOT HEADLESS))
  set(PLATFORM_INPUT_DRIVER_SOURCES src/input/JoystickInputDriver.cc)
  add_definitions(-DENABLE_JOYSTICK)
endif()

set(INPUT_DRIVER_SOURCES
  ${PLATFORM_INPUT_DRIVER_SOURCES}
  ${INPUT_DRIVER_DBUS_SOURCES})

set(GUI_SOURCES
  ${GUI_SOURCES}
  src/cgalworker.cc
  src/editor.cc
  src/initConfig.cc
  src/launchingscreen.cc
  src/mainwin.cc
  src/mouseselector.cc
  src/scadapi.cc
  src/scadlexer.cpp
  src/scintillaeditor.cpp
  src/settings.cc
  src/tabmanager.cc
  src/tabwidget.cc
  src/AutoUpdater.cc
  src/Console.cc
  src/Dock.cc
  src/ErrorLog.cc
  src/EventFilter.h
  src/FontListDialog.cc
  src/FontListTableView.cc
  src/LibraryInfoDialog.cc
  src/OctoPrint.cc
  src/OpenCSGWarningDialog.cc
  src/OpenSCADApp.cc
  src/Preferences.cc
  src/PrintInitDialog.cc
  src/PrintService.cc
  src/ProgressWidget.cc
  src/QGLView.cc
  src/QSettingsCached.cc
  src/QWordSearchField.cc
  src/SettingsWriter.cc
  src/UIUtils.cc
  src/WindowManager.cc
  src/parameter/groupwidget.cpp
  src/parameter/ignoreWheelWhenNotFocused.cpp
  src/parameter/parametercheckbox.cpp
  src/parameter/parametercombobox.cpp
  src/parameter/parameterextractor.cpp
  src/parameter/parameterobject.cpp
  src/parameter/parameterslider.cpp
  src/parameter/parameterspinbox.cpp
  src/parameter/parametertext.cpp
  src/parameter/parametervector.cpp
  src/parameter/parametervirtualwidget.cpp
  src/parameter/ParameterWidget.cc
  src/input/AxisConfigWidget.cc
  src/input/ButtonConfigWidget.cc
  src/input/InputDriver.cc
  src/input/InputDriverManager.cc
  src/input/InputEventMapper.cc
  src/input/WheelIgnorer.cc
  ${INPUT_DRIVER_SOURCES}
  )

# header-only code
set(GUI_HEADERS
  src/AboutDialog.h
  src/MainWindow.h
  src/Network.h
  src/NetworkSignal.h
  )

file(GLOB UIs src/*.ui)
file(GLOB Resources *.qrc)

set(Sources src/openscad.cc ${CORE_SOURCES} ${CGAL_SOURCES} ${OFFSCREEN_SOURCES} ${Resources})
if(HEADLESS)
  add_definitions(-DOPENSCAD_NOGUI)
else()
  list(APPEND Sources ${GUI_SOURCES} ${GUI_HEADERS})
endif()

if (SNAPSHOT)
  add_definitions(-DOPENSCAD_SNAPSHOT)
  set(RESOURCE_FILES icons/icon-nightly.icns)
else()
  set(RESOURCE_FILES icons/OpenSCAD.icns)
endif()

if(ENABLE_SPNAV)
  add_definitions(-DENABLE_SPNAV)
  list(APPEND Sources src/input/SpaceNavInputDriver.cc)
endif()

add_executable(OpenSCAD ${Sources} ${RESOURCE_FILES})

if(APPLE)
  set_target_properties(OpenSCAD PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_ICON_FILE OpenSCAD.icns
    MACOSX_BUNDLE_BUNDLE_VERSION 2017.03
    MACOSX_BUNDLE_SHORT_VERSION_STRING 2017.03
    RESOURCE "${RESOURCE_FILES}"
  )
elseif(UNIX)
  set_target_properties(OpenSCAD PROPERTIES
    OUTPUT_NAME openscad
  )
elseif(MSVC)
  set_target_properties(OpenSCAD PROPERTIES
    LINK_FLAGS "-subsystem:windows -ENTRY:mainCRTStartup -stack:8388608"
  )
endif()

if(ENABLE_SPNAV)
  target_link_libraries(OpenSCAD spnav)
endif()

target_link_libraries(OpenSCAD PRIVATE ${COMMON_LIBRARIES} ${PLATFORM_LIBS})
if(NOT HEADLESS)
  target_link_libraries(OpenSCAD PRIVATE Qt5::Core Qt5::Widgets Qt5::Multimedia Qt5::OpenGL Qt5::Concurrent Qt5::Network ${QT5QSCINTILLA_LIBRARY} ${Qt5DBus_LIBRARIES} ${Qt5Gamepad_LIBRARIES})
endif()

if(INFO)
  include(info)
endif()

add_subdirectory(tests)

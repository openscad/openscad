#
# CMake build system for OpenSCAD
#
# Configuration variables
#   -DHEADLESS=<ON|OFF>
#
#

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
endif()
cmake_minimum_required(VERSION 3.2.2)


if(HEADLESS)
  list(APPEND CONFIG_OPTIONS "HEADLESS")
endif(HEADLESS)
message(STATUS "Configuration: ${CONFIG_OPTIONS}")

project(openscad)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/objects")
set(AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/objects")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE not specified.  Defaulting to 'Release'")
  message(STATUS "Usage: cmake -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel] .")
  set(CMAKE_BUILD_TYPE Release)
else()
  message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

# cmake default flags include -DNDEBUG for non-Debug builds.
# NDEBUG is known to cause OpenSCAD to crash in some case, so make sure it is unset.  Issue #2180 for details.
add_compile_options("-UNDEBUG")

set(CMAKE_CXX_STANDARD 11)
if(APPLE)
  # Needed for deployment target 10.8 as cmake doesn't know about the libc++
  set(CMAKE_CXX_FLAGS "-stdlib=libc++")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_PREFIX_PATH "$ENV{OPENSCAD_LIBRARIES}")

find_package(Eigen3 REQUIRED QUIET)
message(STATUS "Eigen3: ${EIGEN3_VERSION_STRING}")
include_directories(${EIGEN3_INCLUDE_DIRS})

find_package(Boost 1.36 REQUIRED COMPONENTS filesystem system thread regex program_options QUIET)
message(STATUS "Boost: ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
include_directories(${Boost_INCLUDE_DIRS})

find_package(CGAL REQUIRED QUIET)
message(STATUS "CGAL: ${CGAL_MAJOR_VERSION}.${CGAL_MINOR_VERSION}")
include_directories(${CGAL_INCLUDE_DIRS})
add_definitions(-DENABLE_CGAL)

find_package(HarfBuzz 0.9.19 REQUIRED QUIET)
message(STATUS "Harfbuzz: ${HARFBUZZ_VERSION}")
include_directories(${HARFBUZZ_INCLUDE_DIRS})

find_package(FontConfig 2.8.0 REQUIRED QUIET)
message(STATUS "Fontconfig: ${FONTCONFIG_VERSION}")

find_package(Freetype 2.4.9 REQUIRED QUIET)
message(STATUS "Freetype: ${FREETYPE_VERSION_STRING}")
include_directories(${FREETYPE_INCLUDE_DIRS})

find_package(GLIB2 2.26 REQUIRED QUIET)
message(STATUS "Glib: ${GLIB2_VERSION}")
include_directories(${GLIB2_INCLUDE_DIRS})

find_package(LibXml2 2.9 REQUIRED QUIET)
message(STATUS "LibXml2: ${LIBXML2_VERSION_STRING}")
include_directories(${LIBXML2_INCLUDE_DIR})

find_package(FLEX REQUIRED QUIET)
message(STATUS "Flex: ${FLEX_VERSION}")

find_package(BISON REQUIRED QUIET)
message(STATUS "Bison: ${BISON_VERSION}")

find_package(LibZip REQUIRED QUIET)
message(STATUS "LibZip: ${LIBZIP_VERSION}")
include_directories(${LIBZIP_INCLUDE_DIR_ZIP})
include_directories(${LIBZIP_INCLUDE_DIR_ZIPCONF})

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

FLEX_TARGET(openscad_lexer src/lexer.l ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lexer.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lexer.hxx)
BISON_TARGET(openscad_parser src/parser.y ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/parser.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/parser.hxx COMPILE_FLAGS "-d -p parser")
ADD_FLEX_BISON_DEPENDENCY(openscad_lexer openscad_parser)

FLEX_TARGET(comment_lexer src/comment_lexer.l ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_lexer.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_lexer.hxx)
BISON_TARGET(comment_parser src/comment_parser.y ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_parser.cxx DEFINES_FILE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/comment_parser.hxx COMPILE_FLAGS "-d -p comment_parser")
ADD_FLEX_BISON_DEPENDENCY(comment_lexer comment_parser)

if(NOT HEADLESS)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTORCC ON)
  find_package(Qt5 COMPONENTS Core Widgets OpenGL Concurrent REQUIRED QUIET)
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
  
#  add_definitions(${Qt5Widgets_DEFINITIONS})
#  add_definitions(${Qt5OpenGL_DEFINITIONS})
#  add_definitions(${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})
#  add_definitions(${Qt5OpenGL_EXECUTABLE_COMPILE_FLAGS})

  find_package(OpenGL REQUIRED QUIET)

  find_package(GLEW REQUIRED QUIET)
  message(STATUS "GLEW: ${GLEW_FOUND}")

  find_package(OpenCSG REQUIRED QUIET)
  message(STATUS "OpenCSG: ${OPENCSG_VERSION_STRING}")
endif()

# NULLGL - Allow us to build without OpenGL(TM). run 'cmake .. -DNULLGL=1'
# Most tests will fail, but it can be used for testing/experiments
if(NULLGL)
  add_definitions(-DNULLGL)
else()
  add_definitions(-DENABLE_OPENCSG)
endif()

add_definitions(-DENABLE_LIBZIP)
add_definitions(-DENABLE_EXPERIMENTAL)

# Stack size 8MB; github issue 116
add_definitions(-DSTACKSIZE=8388608)

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

#
# Platform specific settings
#
if(APPLE)
  message(STATUS "Offscreen OpenGL Context - using Apple CGL")
  set(PLATFORM_SOURCES src/PlatformUtils-mac.mm src/OffscreenContextCGL.mm src/imageutils-macosx.cc src/CocoaUtils.mm src/AppleEvents.cc)
  find_library(COCOA_LIBRARY Cocoa)
  set(PLATFORM_LIBS ${COCOA_LIBRARY})
elseif(UNIX)
  message(STATUS "Offscreen OpenGL Context - using Unix GLX on X11")
  set(PLATFORM_SOURCES src/OffscreenContextGLX.cc src/imageutils-lodepng.cc src/PlatformUtils-posix.cc)
  find_library(X11_LIBRARY X11)
  set(PLATFORM_LIBS ${X11_LIBRARY})
elseif(WIN32)
  message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
  set(PLATFORM_SOURCES src/OffscreenContextWGL.cc src/imageutils-lodepng.cc src/PlatformUtils-win.cc)
endif()

set(CORE_SOURCES
  src/parsersettings.cc
  src/linalg.cc
  src/colormap.cc
  src/Camera.cc
  src/handle_dep.cc 
  src/value.cc 
  src/calc.cc 
  src/grid.cc 
  src/hash.cc 
  src/expr.cc 
  src/func.cc 
  src/function.cc 
  src/stackcheck.cc 
  src/localscope.cc 
  src/module.cc 
  src/FileModule.cc 
  src/UserModule.cc 
  src/GroupModule.cc 
  src/AST.cc 
  src/ModuleInstantiation.cc 
  src/ModuleCache.cc 
  src/StatCache.cc
  src/node.cc 
  src/NodeVisitor.cc 
  src/context.cc 
  src/builtincontext.cc
  src/modcontext.cc 
  src/evalcontext.cc 
  src/feature.cc
  src/csgnode.cc 
  src/CSGTreeNormalizer.cc 
  src/Geometry.cc 
  src/Polygon2d.cc 
  src/csgops.cc 
  src/transform.cc 
  src/color.cc 
  src/primitives.cc 
  src/projection.cc 
  src/cgaladv.cc 
  src/surface.cc 
  src/control.cc 
  src/render.cc 
  src/rendersettings.cc 
  src/settings.cc
  src/dxfdata.cc 
  src/dxfdim.cc 
  src/offset.cc 
  src/linearextrude.cc 
  src/rotateextrude.cc 
  src/text.cc 
  src/printutils.cc 
  src/fileutils.cc 
  src/progress.cc 
  src/boost-utils.cc 
  src/FontCache.cc
  src/DrawingCallback.cc
  src/FreetypeRenderer.cc
  src/ext/lodepng/lodepng.cpp
  src/PlatformUtils.cc 
  src/libsvg/circle.cc
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
  src/libsvg/transformation.cc
  src/libsvg/util.cc
  src/clipper-utils.cc
  src/assignment.cc
  src/annotation.cc 
  src/ext/polyclipping/clipper.cpp
  ${PLATFORM_SOURCES}
  ${FLEX_openscad_lexer_OUTPUTS}
  ${BISON_openscad_parser_OUTPUTS}
  ${FLEX_comment_lexer_OUTPUTS}
  ${BISON_comment_parser_OUTPUTS})

set(NOCGAL_SOURCES
  src/builtin.cc 
  src/import.cc
  src/import_stl.cc
  src/import_amf.cc
  src/import_off.cc
  src/import_svg.cc
  src/export.cc
  src/export_stl.cc
  src/export_amf.cc
  src/export_off.cc
  src/export_dxf.cc
  src/export_svg.cc
  src/LibraryInfo.cc
  src/polyset.cc
  src/polyset-gl.cc
  src/polyset-utils.cc
  src/GeometryUtils.cc)

set(CGAL_SOURCES
  ${NOCGAL_SOURCES}
  src/CSGTreeEvaluator.cc 
  src/CGAL_Nef_polyhedron.cc 
  src/export_nef.cc
  src/import_nef.cc
  src/cgalutils.cc 
  src/cgalutils-applyops.cc 
  src/cgalutils-project.cc 
  src/cgalutils-tess.cc 
  src/cgalutils-polyhedron.cc 
  src/CGALCache.cc
  src/Polygon2d-CGAL.cc
  src/svg.cc
  src/GeometryEvaluator.cc)

include_directories("src/ext/libtess2/Include")
set(COMMON_SOURCES
  src/nodedumper.cc 
  src/GeometryCache.cc 
  src/clipper-utils.cc 
  src/Tree.cc
  src/ext/polyclipping/clipper.cpp
  src/ext/libtess2/Source/bucketalloc.c
  src/ext/libtess2/Source/dict.c
  src/ext/libtess2/Source/geom.c
  src/ext/libtess2/Source/mesh.c
  src/ext/libtess2/Source/priorityq.c
  src/ext/libtess2/Source/sweep.c
  src/ext/libtess2/Source/tess.c
  src/Tree.cc)

#
# Offscreen OpenGL context source code
#
set(OFFSCREEN_SOURCES
  src/GLView.cc
  src/OffscreenView.cc
  src/imageutils.cc
  src/fbo.cc
  src/system-gl.cc
  src/export_png.cc
  src/CGALRenderer.cc
  src/ThrownTogetherRenderer.cc
  src/renderer.cc
  src/render.cc
  src/OpenCSGRenderer.cc)

set(GUI_SOURCES
  src/mainwin.cc
  src/OpenSCADApp.cc
  src/EventFilter.h
  src/WindowManager.cc
  src/Preferences.cc
  src/FontListDialog.cc
  src/FontListTableView.cc
  src/LibraryInfoDialog.cc
  src/OpenCSGWarningDialog.cc
  src/ProgressWidget.cc
  src/AutoUpdater.cc
  src/QGLView.cc
  src/Dock.cc
  src/UIUtils.cc
  src/scadlexer.cpp
  src/highlighter.cc
  src/cgalworker.cc
  src/editor.cc
  src/legacyeditor.cc
  src/scintillaeditor.cpp
  src/launchingscreen.cc
  src/QWordSearchField.cc
  src/QSettingsCached.cc
  src/comment.cpp
  src/parameter/ParameterWidget.cc
  src/parameter/groupwidget.cpp
  src/parameter/parametercheckbox.cpp
  src/parameter/parametercombobox.cpp
  src/parameter/parameterextractor.cpp
  src/parameter/parameterobject.cpp
  src/parameter/parameterset.cpp
  src/parameter/parameterslider.cpp
  src/parameter/parameterspinbox.cpp
  src/parameter/parametertext.cpp
  src/parameter/parametervector.cpp
  src/parameter/parametervirtualwidget.cpp
  )

# header-only code
set(GUI_HEADERS
  src/AboutDialog.h
  src/MainWindow.h
  )

if(NULLGL)
  message(STATUS "NULLGL is set. Overriding previous OpenGL(TM) settings")
  set(OFFSCREEN_SOURCES
    src/NULLGL.cc # contains several 'nullified' versions of above .cc files
    src/OffscreenView.cc
    src/OffscreenContextNULL.cc
    src/export_png.cc
    src/${OFFSCREEN_IMGUTILS_SOURCE}
    src/imageutils.cc
    src/renderer.cc
    src/render.cc)
endif()

#file(GLOB Headers src/*.h src/*.hpp src/polyclipping/*.hpp)
#list(REMOVE_ITEM Headers ${CMAKE_SOURCE_DIR}/src/SparkleAutoUpdater.h)

file(GLOB UIs src/*.ui)
file(GLOB Resources *.qrc)

set(Sources src/openscad.cc ${CORE_SOURCES} ${COMMON_SOURCES} ${CGAL_SOURCES} ${OFFSCREEN_SOURCES} ${Resources})
if(HEADLESS)
  add_definitions(-DOPENSCAD_NOGUI)
else()
  list(APPEND Sources ${GUI_SOURCES} ${GUI_HEADERS})
endif()

set(RESOURCE_FILES icons/OpenSCAD.icns)

add_executable(OpenSCAD ${Sources} ${RESOURCE_FILES})

# FIXME: snapshot, ICON = icons/icon-nightly.icns

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
endif()

set(COMMON_LIBRARIES
    ${CGAL_LIBRARY}
    ${GMP_LIBRARIES}
    ${MPFR_LIBRARIES}
    ${Boost_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBZIP_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${OPENCSG_LIBRARY}
    ${PLATFORM_LIBS})

target_link_libraries(OpenSCAD ${COMMON_LIBRARIES})
if(NOT HEADLESS)
  target_link_libraries(OpenSCAD Qt5::Core Qt5::Widgets Qt5::OpenGL Qt5::Concurrent ${QT5QSCINTILLA_LIBRARY})
endif()


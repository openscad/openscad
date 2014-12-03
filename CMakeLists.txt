cmake_minimum_required(VERSION 3.0.0)

project(openscad)

include(CheckIncludeFileCXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(src)

find_package(PkgConfig)
pkg_check_modules(PC_EIGEN3 REQUIRED eigen3>=3.0)
add_definitions(${PC_EIGEN3_CFLAGS})
include_directories(${PC_EIGEN3_INCLUDES})

pkg_check_modules(PC_FONTCONFIG REQUIRED fontconfig>=2.8.0)
include_directories(${PC_FONTCONFIG_INCLUDES})

pkg_check_modules(PC_FREETYPE2 REQUIRED freetype2>=2.4.9)
add_definitions(${PC_FREETYPE2_CFLAGS})
include_directories(${PC_FREETYPE2_INCLUDES})

pkg_check_modules(PC_HARFBUZZ REQUIRED harfbuzz>=0.9.19)
add_definitions(${PC_HARFBUZZ_CFLAGS})
include_directories(${PC_HARFBUZZ_INCLUDES})

pkg_check_modules(PC_GLIB REQUIRED glib-2.0>=2.26)
add_definitions(${PC_GLIB_CFLAGS})
include_directories(${PC_GLIB_INCLUDES})

find_package(Boost 1.36 REQUIRED COMPONENTS filesystem system thread regex program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
# The COMPILE_FLAGS and forced C++ compiler is just to be compatible with qmake
if (WIN32)
  set(FLEX_UNISTD_FLAG "-DYY_NO_UNISTD_H")
endif()
FLEX_TARGET(OpenSCADlexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Plexer ${FLEX_UNISTD_FLAG}")
BISON_TARGET(OpenSCADparser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c COMPILE_FLAGS "-p parser")
ADD_FLEX_BISON_DEPENDENCY(OpenSCADlexer OpenSCADparser)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c PROPERTIES LANGUAGE "CXX")

if(NOT HEADLESS)
  find_package(Qt5Widgets REQUIRED)
  include_directories(${Qt5Widgets_INCLUDES})

  find_package(Qt5OpenGL REQUIRED)
  include_directories(${Qt5OpenGL_INCLUDES})

  if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()

  add_definitions(${Qt5Widgets_DEFINITIONS})
  add_definitions(${Qt5OpenGL_DEFINITIONS})
  add_definitions(${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})
  add_definitions(${Qt5OpenGL_EXECUTABLE_COMPILE_FLAGS})

  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
endif()

# NULLGL - Allow us to build without OpenGL(TM). run 'cmake .. -DNULLGL=1'
# Most tests will fail, but it can be used for testing/experiments
if(NULLGL)
  add_definitions("-DNULLGL")
else()
  add_definitions("-DENABLE_OPENCSG")
endif()
add_definitions("-DENABLE_CGAL -DOPENSCAD_YEAR=2014 -DOPENSCAD_MONTH=12 -DOPENSCAD_DAY=3")

#
# Platform specific settings
#
if(APPLE)
  message(STATUS "Offscreen OpenGL Context - using Apple CGL")
  set(OFFSCREEN_CTX_SOURCE "OffscreenContextCGL.mm" CACHE TYPE STRING)
  set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-macosx.cc" CACHE TYPE STRING)
  set(PLATFORMUTILS_SOURCE "PlatformUtils-mac.mm" CACHE TYPE STRING)
elseif(UNIX)
  message(STATUS "Offscreen OpenGL Context - using Unix GLX")
  set(OFFSCREEN_CTX_SOURCE "OffscreenContextGLX.cc" CACHE TYPE STRING)
  set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-lodepng.cc" CACHE TYPE STRING)
  set(PLATFORMUTILS_SOURCE "PlatformUtils-posix.cc" CACHE TYPE STRING)
elseif(WIN32)
  message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
  set(OFFSCREEN_CTX_SOURCE "OffscreenContextWGL.cc" CACHE TYPE STRING)
  set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-lodepng.cc" CACHE TYPE STRING)
  set(PLATFORMUTILS_SOURCE "PlatformUtils-win.cc" CACHE TYPE STRING)
endif()

set(CORE_SOURCES
  src/parsersettings.cc
  src/mathc99.cc
  src/linalg.cc
  src/colormap.cc
  src/Camera.cc
  src/handle_dep.cc
  src/value.cc
  src/calc.cc
  src/expr.cc
  src/func.cc
  src/localscope.cc
  src/module.cc
  src/ModuleCache.cc
  src/node.cc
  src/context.cc
  src/modcontext.cc
  src/evalcontext.cc
  src/feature.cc
  src/csgterm.cc
  src/csgtermnormalizer.cc
  src/Geometry.cc
  src/Polygon2d.cc
  src/polyset.cc
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
  src/lodepng.cpp
  src/PlatformUtils.cc
  src/${PLATFORMUTILS_SOURCE}
  ${FLEX_OpenSCADlexer_OUTPUTS}
  ${BISON_OpenSCADparser_OUTPUTS})

set(NOCGAL_SOURCES
  src/builtin.cc
  src/import.cc
  src/export.cc
  src/LibraryInfo.cc)

set(CGAL_SOURCES
  ${NOCGAL_SOURCES}
  src/CSGTermEvaluator.cc
  src/CGAL_Nef_polyhedron.cc
  src/cgalutils.cc
  src/cgalutils-tess.cc
  src/cgalutils-tess-old.cc
  src/CGALCache.cc
  src/CGAL_Nef_polyhedron_DxfData.cc
  src/Polygon2d-CGAL.cc
  src/polyset-utils.cc
  src/svg.cc
  src/GeometryEvaluator.cc)

set(COMMON_SOURCES
  src/nodedumper.cc
  src/traverser.cc
  src/GeometryCache.cc
  src/clipper-utils.cc
  src/polyclipping/clipper.cpp
  src/Tree.cc)

#
# Offscreen OpenGL context source code
#
set(OFFSCREEN_SOURCES
  src/GLView.cc
  src/OffscreenView.cc
  src/${OFFSCREEN_CTX_SOURCE}
  src/${OFFSCREEN_IMGUTILS_SOURCE}
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
  src/Preferences.cc
  src/FontListDialog.cc
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
  src/launchingscreen.cc)

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

if(HEADLESS)
  message(STATUS "HEADLESS is set.")
  set(GUI_SOURCES "")
endif()

#file(GLOB Headers src/*.h src/*.hpp src/polyclipping/*.hpp)
#list(REMOVE_ITEM Headers ${CMAKE_SOURCE_DIR}/src/SparkleAutoUpdater.h)

file(GLOB UIs src/*.ui)
file(GLOB Resources *.qrc)

set(Sources src/openscad.cc ${CORE_SOURCES} ${COMMON_SOURCES} ${CGAL_SOURCES} ${OFFSCREEN_SOURCES} ${GUI_SOURCES})

add_executable(openscad ${Sources} ${Headers} ${ResourceSources} ${FLEX_OpenSCADlexer_OUTPUTS} ${BISON_OpenSCADparser_OUTPUTS})

if(HEADLESS)
  add_definitions("-DOPENSCAD_TESTING")
  target_link_libraries(openscad
    -lCGAL
    -lgmp
    -lmpfr
    ${PC_HARFBUZZ_LIBRARIES}
    ${PC_FREETYPE2_LIBRARIES}
    ${PC_FONTCONFIG_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PC_GLIB_LIBRARIES})
else()
  qt5_add_resources(ResourceSources ${Resources})
  qt5_use_modules(openscad Widgets OpenGL)
  target_link_libraries(openscad
    ${QT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    -lCGAL
    -lopencsg
    -lgmp
    -lmpfr
    -lqt5scintilla2
    ${PC_HARFBUZZ_LIBRARIES}
    ${PC_FREETYPE2_LIBRARIES}
    ${PC_FONTCONFIG_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PC_GLIB_LIBRARIES})
endif()


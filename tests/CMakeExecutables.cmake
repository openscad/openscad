# make executables for tests

# Internal includes

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/libtess2/Include)

# Handle OpenSCAD version based on VERSION env. variable.
# Use current timestamp if not specified (development builds)
if ("$ENV{VERSION}" STREQUAL "")
  # Timestamp is only available in cmake >= 2.8.11  
  if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.10)
    string(TIMESTAMP VERSION "%Y.%m.%d")
  else()
    set(VERSION "2015.03")
  endif()
else()
  set(VERSION $ENV{VERSION})
endif()
message(STATUS "OpenSCAD version: ${VERSION}")
string(REPLACE "-" ";" SPLITVERSION ${VERSION})
list(GET SPLITVERSION 0 OPENSCAD_SHORTVERSION)
string(REGEX MATCHALL "^[0-9]+|[0-9]+|[0-9]+$" MYLIST "${OPENSCAD_SHORTVERSION}")
list(GET MYLIST 0 OPENSCAD_YEAR)
list(GET MYLIST 1 OPENSCAD_MONTH)
math(EXPR OPENSCAD_MONTH ${OPENSCAD_MONTH}) # get rid of leading zero
list(LENGTH MYLIST VERSIONLEN)
if (${VERSIONLEN} EQUAL 3)
  list(GET MYLIST 2 OPENSCAD_DAY)
  math(EXPR OPENSCAD_DAY ${OPENSCAD_DAY}) # get rid of leading zero
endif()

add_definitions(-DOPENSCAD_VERSION=${VERSION} -DOPENSCAD_SHORTVERSION=${OPENSCAD_SHORTVERSION} -DOPENSCAD_YEAR=${OPENSCAD_YEAR} -DOPENSCAD_MONTH=${OPENSCAD_MONTH})
if (DEFINED OPENSCAD_DAY)
  add_definitions(-DOPENSCAD_DAY=${OPENSCAD_DAY})
endif()

add_definitions(-DENABLE_EXPERIMENTAL -DOPENSCAD_NOGUI)

set(ENABLE_OPENCSG_FLAG "-DENABLE_OPENCSG")

# Platform specific settings

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
	# X11 needed for Offscreen OpenGL on current Un*x, see github issue 1355
	set(OFFSCREEN_CTX_LIBRARIES ${X11_LIBRARIES} CACHE TYPE STRING)
elseif(WIN32)
	message(STATUS "Offscreen OpenGL Context - using Microsoft WGL")
	set(OFFSCREEN_CTX_SOURCE "OffscreenContextWGL.cc" CACHE TYPE STRING)
	set(OFFSCREEN_IMGUTILS_SOURCE "imageutils-lodepng.cc" CACHE TYPE STRING)
	set(PLATFORMUTILS_SOURCE "PlatformUtils-win.cc" CACHE TYPE STRING)
endif()

set(CORE_SOURCES
  tests-common.cc 
  ../src/parsersettings.cc
  ../src/linalg.cc
  ../src/colormap.cc
  ../src/Camera.cc
  ../src/handle_dep.cc 
  ../src/value.cc 
  ../src/calc.cc 
  ../src/grid.cc 
  ../src/hash.cc 
  ../src/expr.cc 
  ../src/func.cc 
  ../src/function.cc 
  ../src/stackcheck.cc 
  ../src/localscope.cc 
  ../src/module.cc 
  ../src/FileModule.cc 
  ../src/UserModule.cc 
  ../src/GroupModule.cc 
  ../src/AST.cc 
  ../src/ModuleInstantiation.cc 
  ../src/ModuleCache.cc 
  ../src/node.cc 
  ../src/NodeVisitor.cc 
  ../src/context.cc 
  ../src/modcontext.cc 
  ../src/evalcontext.cc 
  ../src/feature.cc
  ../src/csgnode.cc 
  ../src/CSGTreeNormalizer.cc 
  ../src/Geometry.cc 
  ../src/Polygon2d.cc 
  ../src/csgops.cc 
  ../src/transform.cc 
  ../src/color.cc 
  ../src/primitives.cc 
  ../src/projection.cc 
  ../src/cgaladv.cc 
  ../src/surface.cc 
  ../src/control.cc 
  ../src/render.cc 
  ../src/rendersettings.cc 
  ../src/dxfdata.cc 
  ../src/dxfdim.cc 
  ../src/offset.cc 
  ../src/linearextrude.cc 
  ../src/rotateextrude.cc 
  ../src/text.cc 
  ../src/printutils.cc 
  ../src/fileutils.cc 
  ../src/progress.cc 
  ../src/boost-utils.cc 
  ../src/FontCache.cc
  ../src/DrawingCallback.cc
  ../src/FreetypeRenderer.cc
  ../src/lodepng.cpp
  ../src/PlatformUtils.cc 
  ../src/libsvg/circle.cc
  ../src/libsvg/ellipse.cc
  ../src/libsvg/group.cc
  ../src/libsvg/libsvg.cc
  ../src/libsvg/line.cc
  ../src/libsvg/path.cc
  ../src/libsvg/polygon.cc
  ../src/libsvg/polyline.cc
  ../src/libsvg/rect.cc
  ../src/libsvg/shape.cc
  ../src/libsvg/svgpage.cc
  ../src/libsvg/transformation.cc
  ../src/libsvg/util.cc
  ../src/clipper-utils.cc 
  ../src/polyclipping/clipper.cpp
  ../src/${PLATFORMUTILS_SOURCE}
  ${FLEX_OpenSCADlexer_OUTPUTS}
  ${BISON_OpenSCADparser_OUTPUTS})

set(NOCGAL_SOURCES
  ../src/builtin.cc 
  ../src/import.cc
  ../src/import_stl.cc
  ../src/import_off.cc
  ../src/import_svg.cc
  ../src/export.cc
  ../src/export_stl.cc
  ../src/export_amf.cc
  ../src/export_off.cc
  ../src/export_dxf.cc
  ../src/export_svg.cc
  ../src/LibraryInfo.cc
  ../src/polyset.cc
  ../src/polyset-gl.cc
  ../src/polyset-utils.cc
  ../src/GeometryUtils.cc)


set(CGAL_SOURCES
  ${NOCGAL_SOURCES}
  ../src/CSGTreeEvaluator.cc 
  ../src/CGAL_Nef_polyhedron.cc 
  ../src/export_nef.cc
  ../src/cgalutils.cc 
  ../src/cgalutils-applyops.cc 
  ../src/cgalutils-project.cc 
  ../src/cgalutils-tess.cc 
  ../src/cgalutils-polyhedron.cc 
  ../src/CGALCache.cc
  ../src/Polygon2d-CGAL.cc
  ../src/svg.cc
  ../src/GeometryEvaluator.cc)

set(COMMON_SOURCES
  ../src/nodedumper.cc 
  ../src/GeometryCache.cc 
  ../src/clipper-utils.cc 
  ../src/Tree.cc
  ../src/polyclipping/clipper.cpp
  ../src/libtess2/Source/bucketalloc.c
  ../src/libtess2/Source/dict.c
  ../src/libtess2/Source/geom.c
  ../src/libtess2/Source/mesh.c
  ../src/libtess2/Source/priorityq.c
  ../src/libtess2/Source/sweep.c
  ../src/libtess2/Source/tess.c
  ../src/Tree.cc)

#
# Offscreen OpenGL context source code
#

set(OFFSCREEN_SOURCES
  ../src/GLView.cc
  ../src/OffscreenView.cc
  ../src/${OFFSCREEN_CTX_SOURCE}
  ../src/${OFFSCREEN_IMGUTILS_SOURCE}
  ../src/imageutils.cc
  ../src/fbo.cc
  ../src/system-gl.cc
  ../src/export_png.cc
  ../src/CGALRenderer.cc
  ../src/ThrownTogetherRenderer.cc
  ../src/renderer.cc
  ../src/render.cc
  ../src/OpenCSGRenderer.cc
)

add_library(tests-core STATIC ${CORE_SOURCES})
target_link_libraries(tests-core ${OPENGL_LIBRARIES} ${GLIB2_LIBRARIES} ${FONTCONFIG_LDFLAGS} ${FREETYPE_LDFLAGS} ${HARFBUZZ_LDFLAGS} ${LIBXML2_LIBRARIES} ${Boost_LIBRARIES} ${COCOA_LIBRARY})

add_library(tests-common STATIC ${COMMON_SOURCES})
target_link_libraries(tests-common tests-core)

add_library(tests-cgal STATIC ${CGAL_SOURCES})
set_target_properties(tests-cgal PROPERTIES COMPILE_FLAGS "${ENABLE_OPENCSG_FLAG} -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(tests-cgal tests-common ${CGAL_LIBRARY} ${CGAL_3RD_PARTY_LIBRARIES} ${MPFR_LIBRARIES} ${GMP_LIBRARIES} )
# GMP must come after MPFR to prevent link errors, see MPFR FAQ 

#
# Create non-CGAL tests
#
add_library(tests-nocgal STATIC ${NOCGAL_SOURCES})
target_link_libraries(tests-nocgal tests-common)
set_target_properties(tests-nocgal PROPERTIES COMPILE_FLAGS "${ENABLE_OPENCSG_FLAG}")

add_library(tests-offscreen STATIC ${OFFSCREEN_SOURCES})
set_target_properties(tests-offscreen PROPERTIES COMPILE_FLAGS "${ENABLE_OPENCSG_FLAG} -DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(tests-offscreen ${OFFSCREEN_CTX_LIBRARIES})

#
# modulecachetest
#
add_executable(modulecachetest modulecachetest.cc)
target_link_libraries(modulecachetest tests-nocgal ${GLEW_LIBRARIES} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})

#
# csgtexttest
#
add_executable(csgtexttest csgtexttest.cc CSGTextRenderer.cc CSGTextCache.cc)
target_link_libraries(csgtexttest tests-nocgal ${GLEW_LIBRARIES} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})

#
# cgalcachetest
#
add_executable(cgalcachetest cgalcachetest.cc)
set_target_properties(cgalcachetest PROPERTIES COMPILE_FLAGS "-DENABLE_CGAL ${CGAL_CXX_FLAGS_INIT}")
target_link_libraries(cgalcachetest tests-cgal ${GLEW_LIBRARIES} ${OPENCSG_LIBRARY} ${APP_SERVICES_LIBRARY})


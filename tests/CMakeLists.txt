cmake_minimum_required(VERSION 2.8)
project(tests)

#
# Build test apps
#

# Qt4
find_package(OpenGL)
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
include(${QT_USE_FILE})
include_directories(../src)

# Eigen2
find_path(EIGEN_INCLUDE_DIRS 
          Eigen/Core 
          PATHS ENV EIGEN2DIR /opt/local/include/eigen2 /usr/include/eigen2)
if (NOT EIGEN_INCLUDE_DIRS)
  message(FATAL_ERROR "Eigen2 not found")
else()
  message(STATUS "Eigen2 found in " ${EIGEN_INCLUDE_DIRS})
endif()
include_directories(${EIGEN_INCLUDE_DIRS})

# Flex/Bison
find_package(BISON)
find_package(FLEX)
# The COMPILE_FLAGS and forced C++ compiler is just to be compatible with qmake
FLEX_TARGET(OpenSCADlexer ../src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp COMPILE_FLAGS "-Plexer")
BISON_TARGET(OpenSCADparser ../src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c COMPILE_FLAGS "-p parser")
ADD_FLEX_BISON_DEPENDENCY(OpenSCADlexer OpenSCADparser)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/parser_yacc.c PROPERTIES LANGUAGE "CXX")


# Internal includes
include_directories(../src)

add_definitions(-DOPENSCAD_VERSION=test)

set(SOURCES dumptest.cc 
           ../src/export.cc 
           ../src/value.cc 
           ../src/expr.cc 
           ../src/func.cc 
           ../src/module.cc 
           ../src/node.cc 
           ../src/context.cc 
           ../src/csgterm.cc 
           ../src/polyset.cc 
           ../src/csgops.cc 
           ../src/transform.cc 
           ../src/primitives.cc 
           ../src/projection.cc 
           ../src/cgaladv.cc 
           ../src/surface.cc 
           ../src/control.cc 
           ../src/render.cc 
           ../src/import.cc 
           ../src/dxfdata.cc 
           ../src/dxftess.cc 
           ../src/dxfdim.cc 
           ../src/dxflinextrude.cc 
           ../src/dxfrotextrude.cc 
           ../src/printutils.cc 
           ../src/progress.cc 
           ../src/nodedumper.cc 
           ../src/traverser.cc 
           ../src/PolySetRenderer.cc 
           ../src/Tree.cc
           ${FLEX_OpenSCADlexer_OUTPUTS}
           ${BISON_OpenSCADparser_OUTPUTS})

add_executable(dumptest ${SOURCES})
target_link_libraries(dumptest ${QT_LIBRARIES} ${OPENGL_LIBRARY})

#
# This functions adds cmd-line tests for those of the FILES which have
# a corresponding TESTNAME-expected.<suffix> file present.
#
function(add_cmdline_test TESTCMD TESTSUFFIX FILES)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  foreach (SCADFILE ${SCAD_FILES})
    get_filename_component(TESTNAME ${SCADFILE} NAME_WE)
# FIXME: Run tests even if no expected files are found (to facilitate generating expected files)
#    if (EXISTS ${CMAKE_SOURCE_DIR}/regression/${TESTCMD_NAME}/${TESTNAME}-expected.${TESTSUFFIX})
      add_test(${TESTCMD_NAME}_${TESTNAME} ${tests_SOURCE_DIR}/test_cmdline_tool.py -s ${TESTSUFFIX} ${CMAKE_BINARY_DIR}/${TESTCMD} ${SCADFILE})
#    endif()
  endforeach()
endfunction()

enable_testing()

# Find all scad files
file(GLOB SCAD_FILES ${CMAKE_SOURCE_DIR}/../testdata/scad/*.scad)

# Add dumptest tests to CTest
add_cmdline_test(dumptest txt ${SCAD_FILES})
# Add csgtexttest tests to CTest
#add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/csgtexttest txt ${SCAD_FILES})
# Add cgaltest tests to CTest
#add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/cgaltest stl ${SCAD_FILES})
# Add dxfexport tests to CTest
#add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/exportdxf dxf ${SCAD_FILES})
# Add csgtermtest tests to CTest
#add_cmdline_test(${CMAKE_SOURCE_DIR}/../test-code/csgtermtest txt ${SCAD_FILES})

# instructions - see ../doc/testing.md

#set(DEBUG_OSCD 1) # print debug info during cmake

cmake_minimum_required(VERSION 3.21)
# Explicitly use new include policy to avoid globally shadowing included modules
# https://cmake.org/cmake/help/latest/policy/CMP0017.html
cmake_policy(SET CMP0017 NEW)
cmake_policy(SET CMP0057 NEW) # Interpret IN_LIST
cmake_policy(SET CMP0070 NEW) # Allow relative paths for file(GENERATE ...)

option(USE_IMAGE_COMPARE_PY "Use built-in image_compare.py" ON)

# Use variables for common paths, to reduce line lengths.

project(OpenSCADTests)
enable_testing()

# Very commonly used cmake-provided paths
set(CBD ${CMAKE_BINARY_DIR}) # build top level
set(CSD ${CMAKE_SOURCE_DIR}) # project top level
set(CCBD ${CMAKE_CURRENT_BINARY_DIR}) # tests build dir
set(CCSD ${CMAKE_CURRENT_SOURCE_DIR}) # tests source dir

function(disable_tests_safe)
    # Only disable tests that actually exist
    set(EXISTING_TESTS)
    foreach(test_name IN LISTS ARGN)
        if(TEST ${test_name})
            list(APPEND EXISTING_TESTS ${test_name})
        endif()
    endforeach()

    # Disable existing tests
    if(EXISTING_TESTS)
        set_tests_properties(${EXISTING_TESTS} PROPERTIES DISABLED TRUE)
    endif()
endfunction()

# When configuring OpenSCADTests as a top-level project, we 
# expect a self-contained OpenSCAD installation to exist.
# The location can be specified via the OPENSCAD_BINARY CMake or
# environment variable.
if(PROJECT_IS_TOP_LEVEL)
   message(STATUS "Configuring tests as a top-level project")
  if(EXISTS "$ENV{OPENSCAD_BINARY}")
    set(OPENSCAD_BINPATH "$ENV{OPENSCAD_BINARY}")
  endif()

  # Release builds are typically built against lib3mf and manifold
  option(ENABLE_LIB3MF_TESTS "Enable lib3mf tests" ON)
  option(ENABLE_MANIFOLD_TESTS "Enable manifold tests" ON)
else() # !PROJECT_IS_TOP_LEVEL
  message(STATUS "Configuring tests in a subdirectory")
  # Very commonly used cmake-provided paths
  set(CBD ${CMAKE_BINARY_DIR}) # build top level
  set(CSD ${CMAKE_SOURCE_DIR}) # project top level
  set(LIBRARIES_DIR "${CSD}/libraries")

  if(APPLE)
    set(OPENSCAD_BINPATH "${CBD}/OpenSCAD.app/Contents/MacOS/OpenSCAD")
  elseif (WIN32 OR MXECROSS)
    set(OPENSCAD_BINPATH "${CBD}/openscad.com")
  elseif(EXISTS "${CBD}/bin/openscad${SUFFIX_WITH_DASH}")
    set(OPENSCAD_BINPATH "${CBD}/bin/openscad${SUFFIX_WITH_DASH}")
  else()
    set(OPENSCAD_BINPATH "${CBD}/openscad${SUFFIX_WITH_DASH}")
  endif()

  # MCAD
  if(NOT EXISTS ${LIBRARIES_DIR}/MCAD/__init__.py)
    message(FATAL_ERROR "MCAD not found. You can install from the OpenSCAD root as follows: \n  git submodule update --init --recursive")
  endif()
  list(APPEND CTEST_ENVIRONMENT "OPENSCADPATH=${LIBRARIES_DIR}")

  find_package(Lib3MF QUIET)
  # Disable LIB3MF tests if library was disabled in build
  if(NOT LIB3MF_FOUND)
    # check again via pkgconfig
    if(NOT MSVC)
      find_package(PkgConfig QUIET)
      if (PKG_CONFIG_FOUND)
        pkg_check_modules(LIB3MF lib3MF)
      endif()
    endif()
  endif()
  if(LIB3MF_FOUND)
    set(ENABLE_LIB3MF_TESTS TRUE)
  endif()
  if(ENABLE_MANIFOLD)
    set(ENABLE_MANIFOLD_TESTS TRUE)
  endif()
endif(PROJECT_IS_TOP_LEVEL)

if(PROJECT_IS_TOP_LEVEL)
  if(NOT OPENSCAD_BINPATH)
    message(FATAL_ERROR "OPENSCAD_BINPATH not set")
  elseif(NOT EXISTS "${OPENSCAD_BINPATH}")
    message(STATUS "Couldn't find the OpenSCAD binary: ${OPENSCAD_BINPATH}")
    message(FATAL_ERROR "Please place an OpenSCAD binary at: ${OPENSCAD_BINPATH}")
    unset(OPENSCAD_BINPATH)
  else() 
    message(STATUS "Found OpenSCAD binary: ${OPENSCAD_BINPATH}")
  endif()
endif()

if(OPENSCAD_BINPATH)
  list(APPEND CTEST_ENVIRONMENT "OPENSCAD_BINARY=${OPENSCAD_BINPATH}")
  # Argument used for import/export tests
  set(OPENSCAD_EXE_ARG "--openscad=${OPENSCAD_BINPATH}")
endif()

if(PROJECT_IS_TOP_LEVEL)
  if(NOT EXAMPLES_DIR)
    get_filename_component(OPENSCAD_EXE_DIR "${OPENSCAD_BINPATH}" DIRECTORY)
    find_path(EXAMPLES_DIR
      NAMES "Basics/logo.scad"
      HINTS
        "${OPENSCAD_EXE_DIR}/../Resources" # macOS app bundle
        "${OPENSCAD_EXE_DIR}/../share/openscad" # (Linux) package install
        "${OPENSCAD_EXE_DIR}" # Windows install
      PATH_SUFFIXES "examples"
      NO_DEFAULT_PATH
    )
  endif()
else()
  set(EXAMPLES_DIR "${CSD}/examples")
endif()

if (NOT EXISTS "${EXAMPLES_DIR}")
  unset(EXAMPLES_DIR)
  message(WARNING "Examples directory not found, disabling examples tests")
endif()

# Test Data paths
set(TEST_DATA_DIR       "${CCSD}/data")
set(TEST_SCAD_DIR       "${CCSD}/data/scad")
set(TEST_CUSTOMIZER_DIR "${CCSD}/data/scad/customizer")
set(TEST_PYTHON_DIR     "${CCSD}/data/python")
# Test runner Python scripts
set(STLEXPORTSANITYTEST_PY   "${CCSD}/stlexportsanitytest.py")
set(EXPORT_IMPORT_PNGTEST_PY "${CCSD}/export_import_pngtest.py")
set(EXPORT_PNGTEST_PY        "${CCSD}/export_pngtest.py")
set(SHOULDFAIL_PY            "${CCSD}/shouldfail.py")
set(TEST_CMDLINE_TOOL_PY     "${CCSD}/test_cmdline_tool.py")

######################
# Check Dependencies #
######################


find_package(Python3 3.4 COMPONENTS Interpreter REQUIRED QUIET)
message(STATUS "Found Python at ${Python3_EXECUTABLE}")

include("cmake/ImageCompare.cmake")
include("cmake/GalliumFixes.cmake")
include("cmake/TestFunctions.cmake")
include("cmake/SetupCTestCustom.cmake")

#################################################
# Configure Templated Files and Generated Tests #
#################################################

configure_file(${TEST_SCAD_DIR}/templates/include-tests-template.scad
               ${TEST_SCAD_DIR}/misc/include-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/use-tests-template.scad
               ${TEST_SCAD_DIR}/misc/use-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/import_stl-tests-template.scad
               ${TEST_SCAD_DIR}/3D/features/import_stl-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/import_3mf-tests-template.scad
               ${TEST_SCAD_DIR}/3D/features/import_3mf-tests.scad)
configure_file(${TEST_SCAD_DIR}/templates/import_dxf-tests-template.scad
               ${TEST_SCAD_DIR}/2D/features/import_dxf-tests.scad)
configure_file(${TEST_PYTHON_DIR}/gen_issue2342-template.py
               ${TEST_PYTHON_DIR}/gen_issue2342.py)
configure_file(${TEST_PYTHON_DIR}/gen_svg_viewbox_tests-template.py
               ${TEST_PYTHON_DIR}/gen_svg_viewbox_tests.py)

# generate a very large scad file which we would rather not commit to the source tree
# this is for stress-testing the parser
add_custom_target(issue2342 ALL
  COMMAND ${Python3_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_issue2342.py ">${TEST_SCAD_DIR}/issues/issue2342.scad"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating issue2342.scad"
)
add_custom_target(svg_viewbox_tests ALL
  COMMAND ${Python3_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_svg_viewbox_tests.py "${TEST_DATA_DIR}/svg/viewbox" "${TEST_SCAD_DIR}/svg/extruded"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating svg viewbox tests"
)

#######################################
# Define Test Output Lines to Exclude #
#######################################

# Ignore "TRACE:   *** Excluding N frames ***" lines globally
ctest_env_append_value(
  OPENSCAD_TEST_EXCLUDE_LINE [[^TRACE:\s*\*\*\* Excluding \d+ frames \*\*\*\s*$]])
# Can add another `ctest_env_append_value(OPENSCAD_TEST_EXCLUDE_LINE ...)` call
# to add another regex to match.

# To debug exclude lines in test, uncomment the following line:
# ctest_env_append_value(OPENSCAD_TEST_EXCLUDE_DEBUG True)

# To tell a specific test to exclude lines that match a regex, call
# add_line_exclusion_for_test after calling add_cmdline_test with same test base
# name. E.g.
#
# # Base name group is foo:
# add_cmdline_test(foo         OPENSCAD SUFFIX foo FILES ${FOO_FILES})
#
# # Assume that FOO_FILES contains a test function bar.
# add_line_exclusion_for_test(foo bar [[regex that matches line(s) to exclude]])
#
# Can add another `add_line_exclusion_for_test(foo bar [[another regex]])` call
# to add another regex to match for same test function.

##################################
# Define Various Test File Lists #
##################################

# Find all scad files
file(GLOB FEATURES_2D_FILES   ${TEST_SCAD_DIR}/2D/features/*.scad)
list(REMOVE_ITEM FEATURES_2D_FILES
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad # -> EXPERIMENTAL_TEXTMETRICS_FILES
)
file(GLOB ISSUES_2D_FILES     ${TEST_SCAD_DIR}/2D/issues/*.scad)
file(GLOB_RECURSE BUGS_2D_FILES    ${TEST_SCAD_DIR}/bugs2D/*.scad)
file(GLOB SCAD_DXF_FILES      ${TEST_SCAD_DIR}/dxf/*.scad
  ${TEST_SCAD_DIR}/misc/utf8-☠-2D.scad
)

file(GLOB SCAD_PDF_FILES      ${TEST_SCAD_DIR}/pdf/*.scad)
if(NOT MINGW)
  # GhostScript on MSYS2/MINGW can't handle the names.
  list(APPEND SCAD_PDF_FILES
    ${TEST_SCAD_DIR}/misc/utf8-☠-2D.scad
  )
endif()

file(GLOB SCAD_SVG_FILES      ${TEST_SCAD_DIR}/svg/svg-spec/*.scad
  ${TEST_SCAD_DIR}/svg/box-w-holes-2d.scad
  ${TEST_SCAD_DIR}/svg/display.scad
  ${TEST_SCAD_DIR}/svg/id-selection-test.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
  ${TEST_SCAD_DIR}/svg/line-cap-line-join.scad
  ${TEST_SCAD_DIR}/svg/simple-center-2d.scad
  ${TEST_SCAD_DIR}/svg/use-transform.scad
  ${TEST_SCAD_DIR}/svg/fill-rule.scad
  ${TEST_SCAD_DIR}/svg/size-percent.scad
  ${TEST_SCAD_DIR}/misc/utf8-☠-2D.scad
  )
if(EXAMPLES_DIR)
  list(APPEND EXAMPLE_2D_FILES
    ${EXAMPLES_DIR}/Old/example015.scad
    ${EXAMPLES_DIR}/Advanced/module_recursion.scad
    ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
    ${EXAMPLES_DIR}/Functions/polygon_areas.scad
    ${EXAMPLES_DIR}/Functions/recursion.scad
  )
endif(EXAMPLES_DIR)

file(GLOB FEATURES_3D_FILES   ${TEST_SCAD_DIR}/3D/features/*.scad)
file(GLOB DEPRECATED_3D_FILES ${TEST_SCAD_DIR}/3D/deprecated/*.scad)
file(GLOB ISSUES_3D_FILES     ${TEST_SCAD_DIR}/3D/issues/*.scad)
file(GLOB SCAD_AMF_FILES           ${TEST_SCAD_DIR}/amf/*.scad)
file(GLOB SCAD_NEF3_FILES          ${TEST_SCAD_DIR}/nef3/*.scad)
file(GLOB FUNCTION_FILES           ${TEST_SCAD_DIR}/functions/*.scad)
file(GLOB REDEFINITION_FILES       ${TEST_SCAD_DIR}/redefinition/*.scad)
file(GLOB_RECURSE BUGS_FILES       ${TEST_SCAD_DIR}/bugs/*.scad)
if(EXAMPLES_DIR)
  file(GLOB_RECURSE EXAMPLE_FILES    ${EXAMPLES_DIR}/*.scad)
  list(REMOVE_ITEM EXAMPLE_FILES
    ${EXAMPLES_DIR}/Basics/roof.scad # -> EXPERIMENTAL_ROOF_FILES
  )
  list(APPEND EXAMPLE_3D_FILES ${EXAMPLE_FILES})
  # Remove 2D files from 3D examples
  list(REMOVE_ITEM EXAMPLE_3D_FILES
    ${EXAMPLES_DIR}/Old/example015.scad
    ${EXAMPLES_DIR}/Advanced/module_recursion.scad
    ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
    ${EXAMPLES_DIR}/Functions/polygon_areas.scad
    ${EXAMPLES_DIR}/Functions/recursion.scad
  )
endif(EXAMPLES_DIR)

list(APPEND MISC_FILES
  ${TEST_SCAD_DIR}/misc/arg-permutations.scad
  ${TEST_SCAD_DIR}/misc/escape-test.scad
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/include-overwrite-main.scad
  ${TEST_SCAD_DIR}/misc/include-overwrite-main2.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
  ${TEST_SCAD_DIR}/misc/special-consts.scad
  ${TEST_SCAD_DIR}/misc/utf8-☠-3D.scad
  ${TEST_SCAD_DIR}/misc/variable-overwrite.scad
)

list(APPEND FAILING_FILES
  ${TEST_SCAD_DIR}/issues/issue1890-comment.scad
  ${TEST_SCAD_DIR}/issues/issue1890-include.scad
  ${TEST_SCAD_DIR}/issues/issue1890-string.scad
  ${TEST_SCAD_DIR}/issues/issue1890-use.scad
)

list(APPEND ECHO_FILES ${FUNCTION_FILES} ${MISC_FILES} ${REDEFINITION_FILES}
  ${TEST_SCAD_DIR}/3D/features/for-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate-parameters.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/misc/expression-evaluation-tests.scad
  ${TEST_SCAD_DIR}/misc/echo-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-fail1-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail2-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail3-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail4-test.scad
  ${TEST_SCAD_DIR}/misc/assert-fail5-test.scad
  ${TEST_SCAD_DIR}/misc/for-c-style-infinite-loop.scad
  ${TEST_SCAD_DIR}/misc/parser-tests.scad
  ${TEST_SCAD_DIR}/misc/builtin-tests.scad
  ${TEST_SCAD_DIR}/misc/dim-all.scad
  ${TEST_SCAD_DIR}/misc/string-test.scad
  ${TEST_SCAD_DIR}/misc/string-indexing.scad
  ${TEST_SCAD_DIR}/misc/string-unicode.scad
  ${TEST_SCAD_DIR}/misc/chr-tests.scad
  ${TEST_SCAD_DIR}/misc/ord-tests.scad
  ${TEST_SCAD_DIR}/misc/vector-values.scad
  ${TEST_SCAD_DIR}/misc/search-tests.scad
  ${TEST_SCAD_DIR}/misc/search-tests-unicode.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-function.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-function2.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-function3.scad
  ${TEST_SCAD_DIR}/misc/recursion-test-module.scad
  ${TEST_SCAD_DIR}/misc/tail-recursion-tests.scad
  ${TEST_SCAD_DIR}/misc/value-reassignment-tests.scad
  ${TEST_SCAD_DIR}/misc/value-reassignment-tests2.scad
  ${TEST_SCAD_DIR}/misc/variable-scope-tests.scad
  ${TEST_SCAD_DIR}/misc/scope-assignment-tests.scad
  ${TEST_SCAD_DIR}/misc/lookup-tests.scad
  ${TEST_SCAD_DIR}/misc/expression-shortcircuit-tests.scad
  ${TEST_SCAD_DIR}/misc/parent_module-tests.scad
  ${TEST_SCAD_DIR}/misc/children-tests.scad
  ${TEST_SCAD_DIR}/misc/range-tests.scad
  ${TEST_SCAD_DIR}/misc/no-break-space-test.scad
  ${TEST_SCAD_DIR}/misc/unicode-tests.scad
  ${TEST_SCAD_DIR}/misc/utf8-tests.scad
  ${TEST_SCAD_DIR}/misc/nbsp-utf8-test.scad
  ${TEST_SCAD_DIR}/misc/nbsp-latin1-test.scad
  ${TEST_SCAD_DIR}/misc/concat-tests.scad
  ${TEST_SCAD_DIR}/misc/include-recursive-test.scad
  ${TEST_SCAD_DIR}/misc/errors-warnings.scad
  ${TEST_SCAD_DIR}/misc/errors-warnings-included.scad
  ${TEST_SCAD_DIR}/misc/children-warnings-tests.scad
  ${TEST_SCAD_DIR}/misc/isundef-test.scad
  ${TEST_SCAD_DIR}/misc/islist-test.scad
  ${TEST_SCAD_DIR}/misc/isnum-test.scad
  ${TEST_SCAD_DIR}/misc/isbool-test.scad
  ${TEST_SCAD_DIR}/misc/isstring-test.scad
  ${TEST_SCAD_DIR}/misc/operators-tests.scad
  ${TEST_SCAD_DIR}/misc/expression-precedence.scad
  ${TEST_SCAD_DIR}/misc/builtins-calling-vec3vec2.scad
  ${TEST_SCAD_DIR}/misc/leaf-module-warnings.scad
  ${TEST_SCAD_DIR}/issues/issue1472.scad
  ${TEST_SCAD_DIR}/misc/empty-stl.scad
  ${TEST_SCAD_DIR}/issues/issue1516.scad
  ${TEST_SCAD_DIR}/issues/issue1528.scad
  ${TEST_SCAD_DIR}/issues/issue1923.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
  ${TEST_SCAD_DIR}/issues/issue1851-each-fail-on-scalar.scad
  ${TEST_SCAD_DIR}/issues/issue2342.scad
  ${TEST_SCAD_DIR}/issues/issue3118-recur-limit.scad
  ${TEST_SCAD_DIR}/issues/issue3541.scad
  ${TEST_SCAD_DIR}/misc/function-scope.scad
  ${TEST_SCAD_DIR}/misc/root-modifiers.scad
  ${TEST_SCAD_DIR}/misc/root-modifier-for.scad
  ${TEST_DATA_DIR}/use-order-test/use-order-test.scad
  ${TEST_SCAD_DIR}/misc/linenumber.scad
)

list(APPEND ASTDUMP_FILES ${MISC_FILES}
  ${TEST_SCAD_DIR}/functions/assert-expression-fail1-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail2-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail3-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/echo-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/expression-precedence-tests.scad
  ${TEST_SCAD_DIR}/functions/let-test-single.scad
  ${TEST_SCAD_DIR}/functions/let-tests.scad
  ${TEST_SCAD_DIR}/functions/list-comprehensions.scad
  ${TEST_SCAD_DIR}/functions/exponent-operator-test.scad
  ${TEST_SCAD_DIR}/misc/ifelse-ast-dump.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
)

list(APPEND DUMP_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES})

list(APPEND RENDER_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND RENDER_3D_FILES ${FEATURES_3D_FILES} ${SCAD_AMF_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES} ${SCAD_NEF3_FILES})
list(APPEND RENDER_3D_FILES
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
  ${TEST_SCAD_DIR}/misc/empty-shape-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
)

# test importing unparseable files, result will be an empty image
list(APPEND STL_IMPORT_FILES
  ${TEST_SCAD_DIR}/stl/stl-import-invalidvertex.scad
  ${TEST_SCAD_DIR}/stl/stl-import-toomanyvertices.scad
  ${TEST_SCAD_DIR}/stl/stl-import-unparseable.scad
  # result will not be empty
  ${TEST_SCAD_DIR}/stl/stl-import-centered.scad
  ${TEST_SCAD_DIR}/stl/stl-import-not-centered.scad
)

list(APPEND OBJ_IMPORT_FILES ${TEST_SCAD_DIR}/obj/obj-import-centered.scad)
list(APPEND 3MF_IMPORT_FILES ${TEST_SCAD_DIR}/3mf/3mf-import-centered.scad)
list(APPEND OFF_IMPORT_FILES ${TEST_SCAD_DIR}/off/off-import-centered.scad)

list(GET RENDER_2D_FILES 0 1 2 RENDERSTDIOTEST_FILES)
list(APPEND ALL_RENDER_FILES ${RENDER_2D_FILES} ${RENDER_3D_FILES} ${BUGS_FILES} ${BUGS_2D_FILES})

list(APPEND PREVIEW_ONLY_FILES
  ${TEST_SCAD_DIR}/3D/features/child-background.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-and-background-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/background-modifier2.scad
  ${TEST_SCAD_DIR}/2D/issues/issue5574.scad
  ${TEST_SCAD_DIR}/misc/utf8-☠-3D.scad
)

list(REMOVE_ITEM ALL_RENDER_FILES 
  # These tests only makes sense in preview mode
  ${PREVIEW_ONLY_FILES}
)

list(APPEND RENDERFORCETEST_FILES
  ${TEST_SCAD_DIR}/3D/issues/issue5548.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135-good.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135-bad.scad
  ${TEST_SCAD_DIR}/3D/misc/import-single-triangle.scad
)

set(PRUNE_TEST ${TEST_SCAD_DIR}/misc/intersection-prune-test.scad)
list(APPEND ALL_PREVIEW_FILES ${3MF_IMPORT_FILES} ${OBJ_IMPORT_FILES} ${OFF_IMPORT_FILES} ${STL_IMPORT_FILES} ${ALL_RENDER_FILES} ${PRUNE_TEST} ${PREVIEW_ONLY_FILES})

list(APPEND COLOR_3D_TEST_FILES
  ${TEST_SCAD_DIR}/misc/color-cubes.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests3.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad
)

# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES
  ${FILES_2D}
  ${SCAD_DXF_FILES}
  ${SCAD_SVG_FILES}
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad
)

# lazy-union
list(APPEND LAZYUNION_3D_FILES
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-objects.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-nested-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-children.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-hull-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-root-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-intersection-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-difference-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-minkowski-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-transform-for.scad
  ${TEST_SCAD_DIR}/3D/features/2d-3d.scad
)
list(APPEND LAZYUNION_2D_FILES
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-2dobjects.scad
)
list(APPEND LAZYUNION_FILES ${LAZYUNION_2D_FILES} ${LAZYUNION_3D_FILES})

list(APPEND SVG_VIEWBOX_TESTS
  viewbox_300x400_none viewbox_600x200_none
  viewbox_300x400_meet_xMinYMin viewbox_300x400_meet_xMidYMin viewbox_300x400_meet_xMaxYMin
  viewbox_600x200_meet_xMinYMin viewbox_600x200_meet_xMinYMid viewbox_600x200_meet_xMinYMax
  viewbox_600x200_slice_xMinYMin viewbox_600x200_slice_xMidYMin viewbox_600x200_slice_xMaxYMin
  viewbox_600x600_slice_xMinYMin viewbox_600x600_slice_xMinYMid viewbox_600x600_slice_xMinYMax
)

# FIXME(kintel): Not in use anymore, but keep for reference?
if(EXAMPLES_DIR)
  list(APPEND SCADFILES_WITH_GREEN_FACES
    ${EXAMPLES_DIR}/Advanced/children.scad
    ${EXAMPLES_DIR}/Basics/CSG-modules.scad
    ${EXAMPLES_DIR}/Basics/CSG.scad
    ${EXAMPLES_DIR}/Basics/logo.scad
    ${EXAMPLES_DIR}/Basics/LetterBlock.scad
    ${EXAMPLES_DIR}/Basics/text_on_cube.scad
    ${EXAMPLES_DIR}/Basics/logo_and_text.scad
    ${EXAMPLES_DIR}/Parametric/sign.scad
    ${EXAMPLES_DIR}/Parametric/candleStand.scad
    ${EXAMPLES_DIR}/Old/example001.scad
    ${EXAMPLES_DIR}/Old/example002.scad
    ${EXAMPLES_DIR}/Old/example003.scad
    ${EXAMPLES_DIR}/Old/example004.scad
    ${EXAMPLES_DIR}/Old/example005.scad
    ${EXAMPLES_DIR}/Old/example006.scad
    ${EXAMPLES_DIR}/Old/example007.scad
    ${EXAMPLES_DIR}/Old/example012.scad
    ${EXAMPLES_DIR}/Old/example016.scad
    ${EXAMPLES_DIR}/Old/example024.scad
  )
endif(EXAMPLES_DIR)
list(APPEND SCADFILES_WITH_GREEN_FACES
  ${TEST_SCAD_DIR}/3D/features/difference-tests.scad
  ${TEST_SCAD_DIR}/3D/features/for-tests.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-erosion.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-difference-test.scad
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1803.scad
  ${TEST_SCAD_DIR}/3D/issues/issue3158.scad
  ${TEST_SCAD_DIR}/3D/issues/issue835.scad
  ${TEST_SCAD_DIR}/3D/issues/issue911.scad
  ${TEST_SCAD_DIR}/3D/issues/issue913.scad
  ${TEST_SCAD_DIR}/3D/issues/issue904.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
  ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
)

list(APPEND SCADFILES_WITH_COLOR
  ${TEST_SCAD_DIR}/3D/features/child-child-test.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests2.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests3.scad
  ${TEST_SCAD_DIR}/3D/features/render-preserve-colors.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate-parameters.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad
  ${TEST_SCAD_DIR}/3D/features/color-names-tests.scad
  ${TEST_SCAD_DIR}/3D/features/hex-colors-tests.scad

  ${TEST_SCAD_DIR}/3D/issues/issue5217.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5738.scad

  #${TEST_SCAD_DIR}/bugs/issue1000.scad
)
if(EXAMPLES_DIR)
  list(APPEND SCADFILES_WITH_COLOR
    ${EXAMPLES_DIR}/Advanced/assert.scad
    ${EXAMPLES_DIR}/Advanced/animation.scad
    ${EXAMPLES_DIR}/Advanced/GEB.scad
    ${EXAMPLES_DIR}/Advanced/children.scad
    ${EXAMPLES_DIR}/Advanced/children_indexed.scad
    ${EXAMPLES_DIR}/Basics/CSG-modules.scad
    ${EXAMPLES_DIR}/Basics/linear_extrude.scad
    ${EXAMPLES_DIR}/Basics/logo_and_text.scad
    ${EXAMPLES_DIR}/Basics/projection.scad
    ${EXAMPLES_DIR}/Basics/text_on_cube.scad
    ${EXAMPLES_DIR}/Advanced/surface_image.scad
    ${EXAMPLES_DIR}/Functions/functions.scad
    ${EXAMPLES_DIR}/Basics/rotate_extrude.scad
    ${EXAMPLES_DIR}/Old/example017.scad
  )
endif(EXAMPLES_DIR)

# Render in CGAL and Manifold should be the same, except for objects with color
# ..but there are some exceptions.
list(APPEND RENDER_DIFFERENT_EXPECTATIONS
  # Manifold backend supports color rendering
  ${SCADFILES_WITH_COLOR}

  # Manifold can repair winding order
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad

  # Manifold can construct geometry which isn't representable in CGAL mode,
  # causing CGAL-based operations like minkowski() to fail.
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # render() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad

  # Manifold can represent the very thin slice
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
)
if(EXAMPLES_DIR)
  list(APPEND RENDER_DIFFERENT_EXPECTATIONS
    ${EXAMPLES_DIR}/Old/example016.scad)
endif(EXAMPLES_DIR)

# Preview in CGAL and Manifold should be the same, except for objects with color
# ..but there are some exceptions.
list(APPEND PREVIEW_DIFFERENT_EXPECTATIONS
  # Manifold can construct geometry which isn't representable in CGAL mode,
  # causing CGAL-based operations like minkowski() to fail.
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # render() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/3D/features/render-preserve-colors.scad

  # resize() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad

  # TODO(kintel): Run bugs files
  # ${TEST_SCAD_DIR}/bugs/issue1000.scad
  # ${TEST_SCAD_DIR}/bugs/issue802.scad
  # ${TEST_SCAD_DIR}/bugs2D/issue2220.scad
)

list(APPEND RENDER_COMMON_FILES ${ALL_RENDER_FILES})
list(REMOVE_ITEM RENDER_COMMON_FILES
  ${SCAD_NEF3_FILES}       # Nef3 import not supported in Manifold mode
  ${RENDER_DIFFERENT_EXPECTATIONS}
)

list(APPEND PREVIEW_COMMON_FILES ${ALL_PREVIEW_FILES})
list(REMOVE_ITEM PREVIEW_COMMON_FILES
  ${PREVIEW_DIFFERENT_EXPECTATIONS}
)

# This is a list of tests that are expected to generate errors when the
# geometry is actually evaluated (because files in the operations are
# inaccessible).  It only needs to be tested against one preview variation.
list(APPEND PREVIEW_ERRORS
  ${TEST_SCAD_DIR}/misc/utf8-import.scad
)

list(APPEND ALL_THROWNTOGETHER_FILES ${ALL_RENDER_FILES})
list(REMOVE_ITEM ALL_THROWNTOGETHER_FILES
  ${PREVIEW_DIFFERENT_EXPECTATIONS}
  ${SCAD_NEF3_FILES}
  ${TEST_SCAD_DIR}/3D/features/color-tests2.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-erosion.scad
  ${TEST_SCAD_DIR}/3D/features/nullspace-difference.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
	${TEST_SCAD_DIR}/3D/issues/issue1803.scad
	${TEST_SCAD_DIR}/3D/issues/issue2090.scad
	${TEST_SCAD_DIR}/3D/issues/issue2841.scad
	${TEST_SCAD_DIR}/3D/issues/issue2841b.scad
)

list(APPEND MANIFOLDHARDWARNING_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-soup.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5741.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5555.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5555b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135-good.scad
)

# Manifoldness corner cases
set(FILES_MANIFOLD_CORNER_CASES
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-slim.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-face-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-face.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-face-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-face.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/minkowski-cubes-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/minkowski-cubes-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/minkowski-polyhedrons-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-vertex.scad
)

# Manifoldness corner cases
set(FILES_CGAL_CORNER_CASES
  ${TEST_SCAD_DIR}/3D/misc/minkowski-cubes-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/minkowski-cubes-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/minkowski-polyhedrons-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-edge.scad
)

##############################
# Define test configurations #
##############################
# Must be done BEFORE adding any tests

# Clear test config cache variables
foreach(CONFIG $CACHE{TEST_CONFIGS})
  unset(${CONFIG}_TEST_CONFIG CACHE)
endforeach()

# Heavy tests are tests taking more than 5 seconds on a decent 2023 desktop
set_test_config(Heavy FILES
  render-csg-cgal_issue267-normalization-crash
  render-cgal_issue267-normalization-crash
)

# Bugs
set_test_config(Bugs FILES ${BUGS_FILES} ${BUGS_2D_FILES} PREFIXES
 render-cgal render-manifold render-csg-cgal preview-cgal preview-manifold throwntogether-cgal throwntogether-manifold)
set_test_config(Bugs FILES ${BUGS_FILES} PREFIXES
  preview-off render-monotone 
  preview-stl 
  render-stl render-stl-manifold
  render-stk-force
  render-off render-force-manifold)

# Re-enable "fixed by manifold" bugs for Manifold
set_test_config(Bugs FILES
  # https://github.com/openscad/openscad/issues/3567
  render-cgal_linear_extrude-scale-zero-tests
  render-csg-cgal_linear_extrude-scale-zero-tests

)

# Remove bugs fixed by manifold from the Bugs tag, to guard against regressions
remove_test_config(Bugs FILES
  render-manifold_issue591
  render-manifold_issue791 
  # FIXME: #802
  render-manifold_issue1455
)

# Examples
if(EXAMPLES_DIR)
  set_test_config(Examples FILES ${EXAMPLE_FILES} PREFIXES
    render-cgal render-manifold
    preview-cgal preview-manifold preview-manifold preview throwntogether-cgal throwntogether-manifold 
    render-csg-cgal render-monotone 
    preview-off render-off dump-examples)
  set_test_config(Examples FILES ${EXAMPLE_2D_FILES} PREFIXES render-dxf)
endif(EXAMPLES_DIR)

# If ctest is run with no configuration specified, then force "Default"
set_directory_properties(PROPERTIES TEST_INCLUDE_FILES "${CCSD}/cmake/EnforceConfig.cmake")

#############
# Add tests #
#############

# Types of tests:
# o echo: Just record console output
# o dump: Export .csg
# o render-cgal: Export to PNG using --render
# o render-force-cgal: Export to PNG using --render=force
# o render-manifold: Export to PNG using --render with --backend=manifold
# o render-force-manifold: Export to PNG using --render=force with --backend=manifold
# o preview-cgal: Export to PNG using OpenCSG
# o preview-manifold: Export to PNG in preview mode with --backend=manifold
# o throwntogether-cgal: Export to PNG using the Throwntogether renderer
# o throwntogether-manifold
# o render-csg-cgal: 1) Export to .csg, 2) import .csg and export to PNG (--render)
# o render-monotone: Same as render-cgal but with the "Monotone" color scheme
# o preview-stl: Export to STL, Re-import and render to PNG (--render)
# o render-stl: Export to STL, Re-import and render to PNG (--render=force)
# o preview-off: Export to OFF, Re-import and render to PNG (--render)
# o render-off: Export to STL, Re-import and render to PNG (--render=force)
# o render-dxf: Export to DXF, Re-import and render to PNG (--render=force)
#

add_cmdline_test(astdump OPENSCAD SUFFIX ast FILES
  ${MISC_FILES}
  ${TEST_SCAD_DIR}/functions/assert-expression-fail1-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail2-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail3-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/bitwise-operators.scad
  ${TEST_SCAD_DIR}/functions/echo-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/expression-precedence-tests.scad
  ${TEST_SCAD_DIR}/functions/let-test-single.scad
  ${TEST_SCAD_DIR}/functions/let-tests.scad
  ${TEST_SCAD_DIR}/functions/list-comprehensions.scad
  ${TEST_SCAD_DIR}/functions/exponent-operator-test.scad
  ${TEST_SCAD_DIR}/misc/ifelse-ast-dump.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
)
add_cmdline_test(astdump-stdio OPENSCAD SUFFIX ast FILES ${TEST_SCAD_DIR}/misc/allexpressions.scad STDIO EXPECTEDDIR astdump ARGS --export-format ast)

add_cmdline_test(csgterm      OPENSCAD SUFFIX term FILES
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
)

add_cmdline_test(echo         OPENSCAD SUFFIX echo FILES ${ECHO_FILES})

# trace-usermodule-parameters is on by default,
# but can generate very long outputs and potentially
# unstable outputs, when combined with recursive tests.
add_cmdline_test(echo         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/recursion-test-vector.scad ARGS --trace-usermodule-parameters=false)

add_cmdline_test(echo-stdio    OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/echo-tests.scad STDIO EXPECTEDDIR echo ARGS --export-format echo)
add_cmdline_test(echo         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/builtin-invalid-range-test.scad ARGS --check-parameter-ranges=on)

# This test is quiet to speed up the test and to have a stable and reproducable output
add_cmdline_test(echo         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/issues/issue4172-echo-vector-stack-exhaust.scad ARGS --quiet --trace-usermodule-parameters=false)

add_cmdline_test(dump           OPENSCAD FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES} SUFFIX csg ARGS)
if(EXAMPLES_DIR)
  add_cmdline_test(dump-examples  OPENSCAD FILES ${EXAMPLE_FILES} SUFFIX csg ARGS)
endif(EXAMPLES_DIR)
# non-ASCII filenames
add_cmdline_test(export-csg-nonascii  OPENSCAD FILES ${TEST_SCAD_DIR}/misc/sfære.scad SUFFIX csg)

add_cmdline_test(render-cgal      OPENSCAD FILES ${RENDER_COMMON_FILES} EXPECTEDDIR render SUFFIX png ARGS --render --backend=cgal)
add_cmdline_test(render-cgal      OPENSCAD FILES ${RENDER_DIFFERENT_EXPECTATIONS} SUFFIX png ARGS --render --backend=cgal)
add_cmdline_test(render-force-cgal OPENSCAD SUFFIX png FILES ${RENDERFORCETEST_FILES} ${FILES_CGAL_CORNER_CASES} ARGS --render=force --backend=cgal)
add_cmdline_test(render-stdio-cgal OPENSCAD SUFFIX png FILES ${RENDERSTDIOTEST_FILES} STDIO EXPECTEDDIR render ARGS --export-format png --render --backend=cgal)
if (ENABLE_MANIFOLD_TESTS)
add_cmdline_test(render-manifold OPENSCAD FILES ${RENDER_COMMON_FILES} EXPECTEDDIR render SUFFIX png ARGS --render --backend=manifold)
add_cmdline_test(render-manifold OPENSCAD FILES ${RENDER_DIFFERENT_EXPECTATIONS} SUFFIX png ARGS --render --backend=manifold)
add_cmdline_test(render-force-manifold      OPENSCAD SUFFIX png FILES ${RENDERFORCETEST_FILES} ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR render ARGS --render=force --backend=manifold)
# This tests that no warnings are issued when using Manifold for converting or processing geometry
add_cmdline_test(render-force-manifold-hardwarnings OPENSCAD SUFFIX png FILES ${MANIFOLDHARDWARNING_FILES} EXPECTEDDIR render ARGS --render=force --backend=manifold --hardwarnings)
endif()

add_cmdline_test(preview-cgal    OPENSCAD FILES ${PREVIEW_COMMON_FILES} EXPECTEDDIR preview SUFFIX png ARGS --backend=cgal)
add_cmdline_test(preview-cgal    OPENSCAD FILES ${PREVIEW_DIFFERENT_EXPECTATIONS} SUFFIX png ARGS --backend=cgal)
add_cmdline_test(preview-cgal    OPENSCAD FILES ${PREVIEW_ERRORS} SUFFIX png ARGS --backend=cgal)
if (ENABLE_MANIFOLD_TESTS)
add_cmdline_test(preview-manifold OPENSCAD SUFFIX png FILES ${PREVIEW_COMMON_FILES} EXPECTEDDIR preview ARGS --backend=manifold)
add_cmdline_test(preview-manifold OPENSCAD SUFFIX png FILES ${PREVIEW_DIFFERENT_EXPECTATIONS} ARGS --backend=manifold)
endif()

add_cmdline_test(throwntogether-cgal  OPENSCAD FILES ${ALL_THROWNTOGETHER_FILES} SUFFIX png EXPECTEDDIR throwntogether ARGS --preview=throwntogether --backend=cgal)
add_cmdline_test(throwntogether-manifold  OPENSCAD FILES ${ALL_THROWNTOGETHER_FILES} SUFFIX png EXPECTEDDIR throwntogether ARGS --preview=throwntogether --backend=manifold)

set(VIEWBOX_TEST "${TEST_SCAD_DIR}/svg/extruded/viewbox-test.scad")
foreach(TEST ${SVG_VIEWBOX_TESTS})
  add_cmdline_test(svgviewbox-${TEST} OPENSCAD ARGS --imgsize 600,600 "-Dfile=\"${TEST_DATA_DIR}/svg/viewbox/${TEST}.svg\";" SUFFIX png FILES ${VIEWBOX_TEST})
endforeach()

add_cmdline_test(svgimport OPENSCAD ARGS --imgsize 600,600 SUFFIX png FILES
  ${TEST_SCAD_DIR}/svg/extruded/box-w-holes.scad
  ${TEST_SCAD_DIR}/svg/extruded/simple-center.scad
)

# 
# OBJECT echo tests
#

file(GLOB OBJECT_TEST ${TEST_SCAD_DIR}/experimental/object/*.scad)
add_cmdline_test(echo EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${OBJECT_TEST} ARGS --enable object-function)


#
# Export/import tests
#

list(APPEND EXPORT_STL_TEST_FILES
  ${TEST_SCAD_DIR}/stl/stl-export.scad
  ${TEST_SCAD_DIR}/misc/utf8-☠-3D.scad
)

# FIXME: Reintroduce
list(APPEND COLOR_EXPORT_TEST_FILES
  ${TEST_SCAD_DIR}/misc/color-export.scad
)

list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-export.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export_dodecahedron.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export_cube.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/3D/features/polyhedron-cube.scad)

list(APPEND EXPORT_3MF_TEST_FILES ${TEST_SCAD_DIR}/3mf/3mf-export.scad)

list(APPEND EXPORT_POV_TEST_FILES ${TEST_SCAD_DIR}/pov/pov-export.scad)

list(APPEND EXPORT_IMPORT_3D_FILES
${TEST_SCAD_DIR}/3D/features/mirror-tests.scad
${TEST_SCAD_DIR}/3D/features/polyhedron-nonplanar-tests.scad
${TEST_SCAD_DIR}/3D/features/rotate_extrude-tests.scad
${TEST_SCAD_DIR}/3D/features/union-coincident-test.scad
${TEST_SCAD_DIR}/3D/issues/fn_bug.scad
${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
${TEST_SCAD_DIR}/3D/issues/issue1215.scad
${TEST_SCAD_DIR}/3D/issues/issue1215b.scad
${TEST_SCAD_DIR}/3D/issues/issue1221.scad
${TEST_SCAD_DIR}/3D/issues/issue1225.scad
${TEST_SCAD_DIR}/3D/issues/issue1258.scad
${TEST_SCAD_DIR}/3D/issues/issue2259.scad
${TEST_SCAD_DIR}/3D/issues/issue5216.scad
${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
${TEST_SCAD_DIR}/misc/preview_variable.scad
)

list(APPEND EXPORT_IMPORT_3D_DIFFERENT_FILES
${TEST_SCAD_DIR}/3D/issues/issue904.scad
${TEST_SCAD_DIR}/3D/issues/issue1105.scad
${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
${TEST_SCAD_DIR}/misc/internal-cavity.scad
${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
)

list(APPEND EXPORT_IMPORT_3D_DIFFERENT_MANIFOLD_FILES
${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
)

#
# There are some caveats with export and import, so we need to test a few combinations:
# 1. It may be possible to export a non-manifold mesh (e.g. malformed polyhedron) due to 
#    no manifoldness checks at export time. This is by design, e.g. to allow users to 
#    troubleshoot externally.
# 2. It may be possible to import such non-manifolds and preview or render them, but it will
#    likely fail when trying to construct a data structure requiring manifold objects (e.g. --render=force)
#
# This leads to three types of tests:
# 1. <format>render: Manifold object export + render (both with --render=force)
# 2. <format>preview: Non-manifold polyhedron export + preview
# 3. <format>render: Complex Manifold polyhedron export + render (both with --render=force)
#    e.g. self-touching polyhedron or malformed but repairable.
#


# Export tests (compare actually exported files)
add_cmdline_test(export-stl              EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render)
add_cmdline_test(export-stl-stdout       EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} STDIO EXPECTEDDIR export-stl ARGS --enable=predictible-output --render --export-format asciistl)
if (ENABLE_MANIFOLD_TESTS)
add_cmdline_test(export-stl-manifold     EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} EXPECTEDDIR export-stl ARGS --enable=predictible-output --backend=manifold --render)
endif()

add_cmdline_test(export-binstl           EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render --export-format binstl)
add_cmdline_test(export-binstl-stdout    EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} STDIO EXPECTEDDIR export-binstl ARGS --enable=predictible-output --render --export-format binstl)

add_cmdline_test(export-obj              EXPERIMENTAL OPENSCAD SUFFIX obj FILES ${EXPORT_OBJ_TEST_FILES} ARGS --enable=predictible-output)
if (ENABLE_LIB3MF_TESTS)
add_cmdline_test(export-3mf              EXPERIMENTAL OPENSCAD SUFFIX 3mf FILES ${EXPORT_3MF_TEST_FILES} ARGS --enable=predictible-output)
endif()
add_cmdline_test(export-pov-as-is        EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold)
add_cmdline_test(export-pov-translate-1  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,0,0,140)
add_cmdline_test(export-pov-translate-2  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=10,0,0,0,0,0,140)
add_cmdline_test(export-pov-translate-3  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,10,0,0,0,0,140)
add_cmdline_test(export-pov-translate-4  EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,10,0,0,0,140)
add_cmdline_test(export-pov-rotate-1     EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,90,0,0,140)
add_cmdline_test(export-pov-rotate-2     EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,90,0,140)
add_cmdline_test(export-pov-rotate-3     EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,0,90,140)
add_cmdline_test(export-pov-distance-1   EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=0,0,0,0,0,0,100)
add_cmdline_test(export-pov-all          EXPERIMENTAL OPENSCAD SUFFIX pov FILES ${EXPORT_POV_TEST_FILES} ARGS --enable=predictible-output --backend=manifold --camera=1,2,3,4,5,6,7)

# Trivial Export/Import files: Sanity-checks bidirectional file format import/export
set(SIMPLE_EXPORT_IMPORT_2D_FILES ${TEST_SCAD_DIR}/misc/square10.scad)
set(SIMPLE_EXPORT_IMPORT_3D_FILES ${TEST_SCAD_DIR}/misc/cube10.scad)
set(SIMPLE_EXPORT_IMPORT_NON_MANIFOLD ${TEST_SCAD_DIR}/3D/misc/polyhedron-single-triangle.scad)

set(EXPORT_IMPORT_2D_RENDER_FILES ${SIMPLE_EXPORT_IMPORT_2D_TESTS} ${FILES_2D})

set(EXPORT_IMPORT_3D_PREVIEW_FILES ${SIMPLE_EXPORT_IMPORT_NON_MANIFOLD} ${SIMPLE_EXPORT_IMPORT_3D_FILES})
set(EXPORT_IMPORT_3D_RENDER_FILES ${SIMPLE_EXPORT_IMPORT_3D_FILES} ${EXPORT_IMPORT_3D_FILES})
list(REMOVE_ITEM EXPORT_IMPORT_3D_RENDER_FILES
  # Non-manifold polyhedrons can never be rendered
  ${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
)
set(EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES ${EXPORT_IMPORT_3D_RENDER_FILES})

# Export-import tests
add_cmdline_test(render-monotone OPENSCAD SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} ${SIMPLE_EXPORT_IMPORT_2D_FILES} ARGS --colorscheme=Monotone --render)
add_cmdline_test(preview-stl SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=STL)
add_cmdline_test(preview-off SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF)
add_cmdline_test(preview-amf SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=AMF)
add_cmdline_test(preview-obj SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OBJ)

add_cmdline_test(render-csg-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${RENDER_COMMON_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --backend=cgal)
add_cmdline_test(render-csg-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${RENDER_DIFFERENT_EXPECTATIONS} EXPECTEDDIR render-cgal ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --backend=cgal)
add_cmdline_test(render-stl SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force)
add_cmdline_test(render-off-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=cgal)
add_cmdline_test(render-off-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=cgal)
add_cmdline_test(render-amf SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=AMF --render=force)
add_cmdline_test(render-obj SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force)

if (ENABLE_MANIFOLD_TESTS)
set(render-off-manifold_FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES})
list(REMOVE_ITEM render-off-manifold_FILES
 ${TEST_SCAD_DIR}/misc/cube10.scad
 ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
)
set(render-3mf-manifold_FILES ${render-off-manifold_FILES})
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_MANIFOLD_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)

add_cmdline_test(render-stl-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force --backend=manifold)
add_cmdline_test(render-stl-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR render-off-manifold ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force --backend=manifold)

add_cmdline_test(render-amf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=AMF --render=force --backend=manifold)
add_cmdline_test(render-obj-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force --backend=manifold)
add_cmdline_test(render-obj-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR render-off-manifold ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force --backend=manifold)
endif(ENABLE_MANIFOLD_TESTS)

if (ENABLE_LIB3MF_TESTS)
add_cmdline_test(preview-3mf         SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF)
add_cmdline_test(render-3mf-cgal     SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=cgal)
add_cmdline_test(render-3mf-cgal     SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=cgal)
add_cmdline_test(render-3mf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
add_cmdline_test(render-3mf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
add_cmdline_test(render-3mf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_MANIFOLD_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
endif()

add_cmdline_test(render-dxf  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_2D_RENDER_FILES} ${SCAD_DXF_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=DXF --render=force)
add_cmdline_test(render-svg  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_2D_RENDER_FILES} ${SCAD_SVG_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=SVG --render=force)

# SVG Export
add_cmdline_test(export-svg SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX svg FILES ${SCAD_SVG_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=SVG)
add_cmdline_test(export-svg-fill-stroke SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX svg FILES ${SCAD_SVG_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=SVG -O export-svg/fill=true -O export-svg/fill-color=cyan -O export-svg/stroke-color=magenta -O export-svg/stroke-width=3)
add_cmdline_test(export-svg-fill-only SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX svg FILES ${SCAD_SVG_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=SVG -O export-svg/fill=true -O export-svg/fill-color=blue -O export-svg/stroke=false)

# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(export-stl-sanitytest  SCRIPT ${STLEXPORTSANITYTEST_PY} SUFFIX txt FILES ${TEST_SCAD_DIR}/misc/normal-nan.scad ARGS ${OPENSCAD_EXE_ARG})

# Export/import color support
add_cmdline_test(offcolorpngtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${COLOR_3D_TEST_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=OFF --backend=manifold --render)
add_cmdline_test(3mfcolorpngtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${COLOR_3D_TEST_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=3MF --backend=manifold --render)

# PDF Export
add_cmdline_test(export-pdf SCRIPT ${EXPORT_PNGTEST_PY} SUFFIX png FILES ${SCAD_PDF_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=PDF -O export-pdf/show-scale=false KERNEL Square:2)
add_cmdline_test(export-pdf-fill SCRIPT ${EXPORT_PNGTEST_PY} SUFFIX png FILES ${SCAD_PDF_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=PDF -O export-pdf/show-scale=false -O export-pdf/fill=true -O export-pdf/fill-color=cyan -O export-pdf/stroke-color=magenta -O export-pdf/stroke-width=1 KERNEL Square:2)

# Expected failing tests
add_failing_test(stlfailedtest         SUFFIX stl  FILES ${TEST_SCAD_DIR}/misc/empty-union.scad ARGS --retval=1)
add_failing_test(offfailedtest         SUFFIX off  FILES ${TEST_SCAD_DIR}/misc/empty-union.scad ARGS --retval=1)
add_failing_test(parsererrors          SUFFIX stl  FILES ${FAILING_FILES} ARGS --retval=1)
# Hardwarning Test
add_failing_test(hardwarnings          SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/errors-warnings.scad ARGS --retval=1 --hardwarnings)

# Verify that test framework is paying attention to alpha channel, issue 1492
#add_cmdline_test(openscad-colorscheme-cornfield-alphafail  ARGS --colorscheme=Cornfield SUFFIX png FILES ${EXAMPLES_DIR}/Basics/logo.scad)

# The "expected image" supplied for this "alphafail" test has the alpha channel for all background pixels cleared (a==0),
# when they should be opaque (a==1) for this colorscheme.
# So if test framework is functioning properly then the image comparison should fail.
# Commented out because the master branch isn't capable of making the expected image yet.
# Also TEST_GENERATE=1 makes an expected image that makes the test fail.
#set_property(TEST openscad-colorscheme-cornfield-alphafail_logo PROPERTY WILL_FAIL TRUE)

#
# Customizer tests
#
set(SET_OF_PARAM_TEST "${TEST_CUSTOMIZER_DIR}/setofparameter.scad")
set(SET_OF_PARAM_JSON "${TEST_CUSTOMIZER_DIR}/setofparameter.json")
add_cmdline_test(customizer OPENSCAD ARGS SUFFIX ast FILES
  ${TEST_CUSTOMIZER_DIR}/description.scad
  ${TEST_CUSTOMIZER_DIR}/parameter.scad
  ${TEST_CUSTOMIZER_DIR}/allmodulescomment.scad
  ${TEST_CUSTOMIZER_DIR}/allfunctionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/allexpressionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/group.scad
)
add_cmdline_test(customizer-first          OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P firstSet)
add_cmdline_test(customizer-wrong          OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P wrongSetValues)
add_cmdline_test(customizer-incomplete     OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P thirdSet)
add_cmdline_test(customizer-imgset         OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P imagine)
add_cmdline_test(customizer-setNameWithDot OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P Name.dot)

# Variable override (-D arg)
add_cmdline_test(openscad-override         OPENSCAD FILES ${TEST_SCAD_DIR}/misc/override.scad SUFFIX echo ARGS -D a=3$<SEMICOLON>)

#
# Camera tests
#
# Test transcription of --camera options to $vp* for 7-argument --camera.
add_cmdline_test(echo-cli-view-variables-7         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)
add_cmdline_test(dump-cli-view-variables-7         OPENSCAD SUFFIX csg FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)
add_cmdline_test(preview-cli-view-variables-7      OPENSCAD SUFFIX png FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)

# Test transcription of --camera options to $vp* for 6-argument --camera.
add_cmdline_test(echo-cli-view-variables-6         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)
add_cmdline_test(dump-cli-view-variables-6         OPENSCAD SUFFIX csg FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)
add_cmdline_test(preview-cli-view-variables-6      OPENSCAD SUFFIX png FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)

set(CAMERA_TEST           "${TEST_SCAD_DIR}/3D/misc/camera-tests.scad")
set(CAMERA_TEST_OFFCENTER "${TEST_SCAD_DIR}/3D/misc/camera-tests-offcenter.scad")
set(CAMERA_TEST_VP        "${TEST_SCAD_DIR}/3D/misc/camera-vp.scad")
# Image output parameters
add_cmdline_test(openscad-imgsize          OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 100,100)
add_cmdline_test(openscad-imgstretch       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 500,100)
add_cmdline_test(openscad-imgstretch2      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 100,500)
# Perspective gimbal cam
set(IMGSIZE "--imgsize=500,500")
add_cmdline_test(openscad-camdist          OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,90,0,90,200)
add_cmdline_test(openscad-camrot           OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,440,337.5,315,200)
add_cmdline_test(openscad-camtrans         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,200)
add_cmdline_test(openscad-camtrans-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,6000 --viewall)
add_cmdline_test(openscad-camtrans-viewall-offcenter OPENSCAD FILES ${CAMERA_TEST_OFFCENTER} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,30,40,50,10 --viewall --autocenter)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-20,90,0,90,220 --projection=o)
add_cmdline_test(openscad-camortho-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o)
# Perspective vector cam
add_cmdline_test(openscad-cameye            OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=120,80,60,0,0,0)
add_cmdline_test(openscad-cameye_front      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,-130,0,0,0,0)
add_cmdline_test(openscad-cameye_back       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,130,0,0,0,0)
add_cmdline_test(openscad-cameye_left       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=-130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_right      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_top        OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,130,0,0,0)
add_cmdline_test(openscad-cameye_bottom     OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,-130,0,0,0)
add_cmdline_test(openscad-cameye2           OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=160,140,130,0,0,0)
add_cmdline_test(openscad-camcenter         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,60,30,20,10,30)
add_cmdline_test(openscad-camcenter-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=60,40,30,20,10,30 --viewall)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=90,80,75,0,0,0 --projection=o)
add_cmdline_test(openscad-cameyeortho-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=16,14,13,0,0,0 --viewall --projection=o)

add_cmdline_test(openscad-camvp-variables     OPENSCAD FILES ${CAMERA_TEST_VP} SUFFIX png ARGS ${IMGSIZE})
add_cmdline_test(openscad-camvp-override      OPENSCAD FILES ${CAMERA_TEST_VP} SUFFIX png ARGS ${IMGSIZE} --camera=120,80,60,0,0,0)

#
# View Options tests
#
set(VIEW_OPTIONS_TEST "${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad")
add_cmdline_test(preview-view-axes              OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes)
add_cmdline_test(preview-view-axes-scales       OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales)
add_cmdline_test(preview-view-axes-scales-edges OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales,edges)
add_cmdline_test(render-view-crosshairs         OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --render --view crosshairs)
set(VIEW_OPTIONS_EDGES_TEST
  ${VIEW_OPTIONS_TEST}
  ${TEST_SCAD_DIR}/misc/cube10.scad
  ${TEST_SCAD_DIR}/3D/features/render-preserve-colors.scad
)
add_cmdline_test(preview-view-edges-cgal           OPENSCAD FILES ${VIEW_OPTIONS_EDGES_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges --backend=cgal)
add_cmdline_test(preview-view-edges-manifold       OPENSCAD FILES ${VIEW_OPTIONS_EDGES_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges --backend=manifold)
add_cmdline_test(render-view-edges-manifold OPENSCAD FILES ${VIEW_OPTIONS_EDGES_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges --render --backend=manifold)

#
# Colorscheme tests
#
if(EXAMPLES_DIR)
  set(LOGO_EXAMPLE ${EXAMPLES_DIR}/Basics/logo.scad)
  set(CSG_EXAMPLE  ${EXAMPLES_DIR}/Basics/CSG.scad)
  add_cmdline_test(openscad-colorscheme-cornfield       OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Cornfield)
  add_cmdline_test(openscad-colorscheme-metallic        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Metallic)
  add_cmdline_test(openscad-colorscheme-sunset          OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Sunset)
  add_cmdline_test(openscad-colorscheme-starnight       OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Starnight)
  add_cmdline_test(openscad-colorscheme-monotone        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=Monotone)
  add_cmdline_test(openscad-colorscheme-clearsky        OPENSCAD FILES ${LOGO_EXAMPLE} SUFFIX png ARGS --colorscheme=ClearSky)
  add_cmdline_test(openscad-colorscheme-metallic-render OPENSCAD FILES ${CSG_EXAMPLE}  SUFFIX png ARGS --colorscheme=Metallic --render)
endif(EXAMPLES_DIR)

############################
# Experimental tests       #
############################

#
# --enable=lazyunion tests
#
add_cmdline_test(lazyunion-export-csg  EXPERIMENTAL OPENSCAD SUFFIX csg FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-preview     EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-render      EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union --render)
add_cmdline_test(lazyunion-render-stl  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=STL --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-render-dxf  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-render ARGS ${OPENSCAD_EXE_ARG} --format=DXF --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-render-svg  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-render ARGS ${OPENSCAD_EXE_ARG} --format=SVG --enable=lazy-union --render=force)

#
# --enable=roof tests
#
if(EXAMPLES_DIR)
  list(APPEND EXPERIMENTAL_ROOF_FILES ${EXAMPLES_DIR}/Basics/roof.scad)
  add_cmdline_test(dump-examples    EXPERIMENTAL OPENSCAD SUFFIX csg FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof)
  add_cmdline_test(render-cgal      EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --render --enable=roof --backend=cgal)
  add_cmdline_test(render-manifold  EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR render ARGS --render --enable=roof --backend=manifold)
  add_cmdline_test(preview-cgal     EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR preview ARGS --enable=roof --backend=cgal)
  add_cmdline_test(preview-manifold EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof --backend=manifold)
  add_cmdline_test(throwntogether   EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --preview=throwntogether --enable=roof)
  add_cmdline_test(render-csg-cgal        EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --enable=roof)
endif(EXAMPLES_DIR)

#
# --enable=import-function tests
#
list(APPEND EXPERIMENTAL_IMPORT_FILES
  ${TEST_SCAD_DIR}/json/import-json.scad
  ${TEST_SCAD_DIR}/json/import-json-relative-path.scad
  )
add_cmdline_test(echo           EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${EXPERIMENTAL_IMPORT_FILES} ARGS --enable=import-function)

#
# --enable=textmetrics tests
#
list(APPEND EXPERIMENTAL_TEXTMETRICS_ECHO_FILES
  ${TEST_SCAD_DIR}/misc/isobject-test.scad
  ${TEST_SCAD_DIR}/misc/text-metrics-test.scad
)
list(APPEND EXPERIMENTAL_TEXTMETRICS_FILES
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad
)
add_cmdline_test(echo           EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${EXPERIMENTAL_TEXTMETRICS_ECHO_FILES} ARGS --enable=textmetrics)
add_cmdline_test(dump           EXPERIMENTAL OPENSCAD SUFFIX csg  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --enable=textmetrics)
add_cmdline_test(preview-cgal   EXPERIMENTAL OPENSCAD SUFFIX png  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR preview ARGS --enable=textmetrics --backend=cgal)
add_cmdline_test(throwntogether EXPERIMENTAL OPENSCAD SUFFIX png  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --preview=throwntogether --enable=textmetrics)
add_cmdline_test(render-csg-cgal      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --enable=textmetrics)
add_cmdline_test(render-cgal    EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR render ARGS --render --enable=textmetrics --backend=cgal)
add_cmdline_test(render-dxf      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png ARGS ${OPENSCAD_EXE_ARG} --format=DXF --render=force --enable=textmetrics EXPECTEDDIR render FILES ${EXPERIMENTAL_TEXTMETRICS_FILES})
add_cmdline_test(render-svg      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png ARGS ${OPENSCAD_EXE_ARG} --format=SVG --render=force --enable=textmetrics EXPECTEDDIR render FILES ${EXPERIMENTAL_TEXTMETRICS_FILES})

#
# --enable=vector-swizzle tests
#
add_cmdline_test(echo EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/vector-swizzling.scad ARGS --enable=vector-swizzle)

############################
# Relative filenames tests #
############################
# These tests writing outputs to a relative filename, and verifies that the
# resulting file is written to the correct location.

function(add_output_file_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "FILE;FORMAT" "" ${ARGN})

  add_test(NAME "${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_run" CONFIGURATIONS Default COMMAND ${OPENSCAD_BINPATH} ${TESTCMD_FILE} -o ${TESTCMD_BASENAME}.${TESTCMD_FORMAT})
  add_test(NAME "${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_check" CONFIGURATIONS Default COMMAND ${CMAKE_COMMAND} -E cat ${TESTCMD_BASENAME}.${TESTCMD_FORMAT})
  set_tests_properties("${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_check" PROPERTIES DEPENDS "${TESTCMD_BASENAME}_${TESTCMD_FORMAT}_run")
endfunction()

add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT stl)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT off)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT wrl)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT amf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT 3mf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT csg)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/2D/features/square-tests.scad FORMAT dxf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/2D/features/square-tests.scad FORMAT svg)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/2D/features/square-tests.scad FORMAT pdf)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT png)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT echo)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT ast)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT term)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT nef3)
add_output_file_test(relative-output FILE ${TEST_SCAD_DIR}/3D/features/cube-tests.scad FORMAT nefdbg)

disable_tests_safe(
  # Disable tests failing due to https://github.com/openscad/openscad/issues/4632
  relative-output_csg_run
  relative-output_csg_check
  relative-output_ast_run 
  relative-output_ast_check

###################################
# Disable Tests with Known Issues #
###################################

  # These don't output anything
  render-dxf_text-empty-tests
  render-dxf_nothing-decimal-comma-separated
  render-dxf_nullspace-2d
  render-svg_text-empty-tests
  render-svg_nullspace-2d

  # z-fighting different on different machines
  preview-cgal_issue1165
  preview-cgal_issue1215
  throwntogether-cgal_issue1089
  throwntogether-manifold_issue1089
  throwntogether-cgal_issue1215
  throwntogether-manifold_issue1215

  # The inf/nan tests fail when exporting CSG and rendering that output again
  # as currently inf/nan is written directly to the CSG file (e.g. r = inf)
  # which is not valid or even misleading in case a variable inf exists.
  # FIXME: define export behavior for inf/nan when exporting CSG files
  # These tests return error code 1.
  # FIXME: We should have a way of running these and verify the return code
  render-csg-cgal_primitive-inf-tests
  render-csg-cgal_transform-nan-inf-tests

  # Triggers a floating point accuracy issue causing loaded .csg to
  # render slightly differently
  render-cgal_nothing-decimal-comma-separated
  render-cgal_import-empty-tests
  render-cgal_empty-shape-tests
  render-csg-cgal_issue1258

  # The Apple SW renderer has a bug in the GLSL dFdy() function, used
  # by GLSL fwidth(), which causes horizonal line artifacts when using
  # wireframe rendering in preview model.
  preview-view-edges-cgal_view-options-tests
  preview-view-edges-manifold_view-options-tests
  render-view-edges-manifold_view-options-tests
  preview-view-axes-scales-edges_view-options-tests

  # CGAL sometimes fail to union minkowski convex decompositions, causing visual differences
  # in rendering.
  render-cgal_minkowski3-difference-test
  render-csg-cgal_minkowski3-difference-test
  preview-cgal_minkowski3-difference-test

  # https://github.com/openscad/openscad/issues/909
  preview-amf_polyhedron-single-triangle
  render-amf_bad-stl-tardis
  render-amf_bad-stl-pcbvicebar
  render-amf_bad-stl-wing

  # When exported from CGAL, the file cannot be re-imported
  render-stl_bad-stl-wing
  render-off-cgal_bad-stl-wing
  render-obj_bad-stl-wing

  # https://github.com/openscad/openscad/issues/2301
  render-cgal_issue2301
  preview-cgal_issue2301
  render-csg-cgal_issue2301
  throwntogether-cgal_issue2301

  # https://github.com/openscad/openscad/issues/2090
  render-cgal_issue2090
  preview-cgal_issue2090
  render-csg-cgal_issue2090

  # https://github.com/openscad/openscad/issues/2841
  render-cgal_issue2841
  preview-cgal_issue2841
  render-csg-cgal_issue2841
  render-cgal_issue2841b
  preview-cgal_issue2841b
  render-csg-cgal_issue2841b
  
  # OpenCSG z fighting
  preview-cgal_surface_image
  throwntogether-cgal_surface_image

  # https://github.com/openscad/openscad/issues/5159
  render-cgal_projection
  render-csg-cgal_projection

  # https://github.com/openscad/openscad/issues/5158
  #render-cgal_example017 
  #throwntogether-cgal_example017
  
  # https://github.com/openscad/openscad/issues/4398
  render-csg-cgal_logo_and_text

  # z-fighting different on different machines
  preview-manifold_issue1165
  preview-manifold_issue1215

  # Manifold triangle order not stable, causes transparent
  # objects to render differently on different machines
  render-manifold_color-tests
  render-manifold_hex-colors-tests

  # https://github.com/openscad/openscad/issues/909
  render-amf-manifold_bad-stl-wing
  render-amf-manifold_bad-stl-pcbvicebar
  render-amf-manifold_bad-stl-tardis

  # https://github.com/openscad/openscad/issues/2841
  render-manifold_issue2841b
  preview-manifold_issue2841b

  # https://github.com/openscad/openscad/issues/5135
  render-force-manifold_issue5135-bad

  # https://github.com/openscad/openscad/issues/5218
  throwntogether-manifold_issue5218
  throwntogether-manifold_issue913

  # z-fighting different on different machines
  preview-cgal_example017
  preview-manifold_example017

  # OpenCSG z fighting
  preview-manifold_surface_image
  throwntogether-manifold_surface_image

  # Manifold triangle order not stable, causes transparent
  # objects to render differently on different machines
  render-manifold_projection

  # https://github.com/openscad/openscad/issues/5218
  throwntogether-manifold_example016

  # https://github.com/openscad/openscad/issues/5220
  render-cgal_roof
  render-csg-cgal_roof

  # https://github.com/openscad/openscad/issues/5966
  render-off-manifold_issue5216

  # For some reason, only failing on Linux
  render-manifold_import_3mf-tests
  preview-cgal_3mf-import-centered
  preview-manifold_3mf-import-centered
  preview-manifold_import_3mf-tests
  render-off-manifold_issue5216
  export-3mf_3mf-export
  render-3mf-cgal_bad-stl-tardis
  render-3mf-cgal_bad-stl-wing
  render-3mf-manifold_issue5216

  preview-cgal_import_3mf-tests
  render-cgal_import_3mf-tests
  render-csg-cgal_import_3mf-tests
  throwntogether-cgal_import_3mf-tests
  throwntogether-manifold_import_3mf-tests
)

list(APPEND NEF3_BROKEN_TESTS
  render-cgal_nef3_broken
  preview-cgal_nef3_broken
  throwntogether-cgal_nef3_broken
  render-csg-cgal_nef3_broken
)

# Platform specific test disable
if(APPLE)
elseif(UNIX)
elseif(WIN32 OR MXECROSS)
  set_tests_properties(
    # Known UTF-8 issue on Windows
    export-csg-nonascii_sfære
    # requires `gs` tool
    export-pdf_centered
    export-pdf_simple-pdf
    PROPERTIES DISABLED TRUE
  )
endif()

if(NOT ENABLE_MANIFOLD)
  disable_tests_safe(
    throwntogether-manifold_issue2301
    export-pov-as-is_pov-export
    export-pov-translate-1_pov-export
    export-pov-translate-2_pov-export
    export-pov-translate-3_pov-export
    export-pov-translate-4_pov-export
    export-pov-rotate-1_pov-export
    export-pov-rotate-2_pov-export
    export-pov-rotate-3_pov-export
    export-pov-distance-1_pov-export
    export-pov-all_pov-export
    render-3mf-manifold_bad-stl-tardis
    render-3mf-manifold_bad-stl-wing
    render-3mf-manifold_issue904
    render-3mf-manifold_issue1105
    render-3mf-manifold_issue1105b
    render-3mf-manifold_issue1105d
    render-3mf-manifold_issue1215c
    render-3mf-manifold_internal-cavity
    render-3mf-manifold_internal-cavity-polyhedron
    render-3mf-manifold_rotate_extrude-hole
    render-3mf-manifold_polyhedron-tests
    offcolorpngtest_color-cubes
    offcolorpngtest_color-tests3
    offcolorpngtest_linear_extrude-parameter-tests
    offcolorpngtest_resize-tests
    3mfcolorpngtest_color-cubes
    3mfcolorpngtest_color-tests3
    3mfcolorpngtest_linear_extrude-parameter-tests
    3mfcolorpngtest_resize-tests
    render-view-crosshairs_view-options-tests
    preview-view-edges-manifold_render-preserve-colors
    render-view-edges-manifold_render-preserve-colors
    openscad-colorscheme-metallic-render_CSG
    lazyunion-render_lazyunion-difference-for
    render-manifold_roof
  ) 
endif()

####################
# Extra Debug Info #
####################

# Use cmake option "--log-level DEBUG" during top level config to see this
message(DEBUG "Available test configurations: ${TEST_CONFIGS}")
foreach(CONF ${TEST_CONFIGS})
  list(SORT ${CONF}_TEST_CONFIG)
  message(DEBUG "${CONF}: ${${CONF}_TEST_CONFIG}")
endforeach()

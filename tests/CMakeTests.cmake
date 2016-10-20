
#
# Tags tests as disabled. This is more convenient than removing them manually
# from the lists of filenames
#
macro(disable_tests)
  foreach (TESTNAME ${ARGN})
#    message("Disabling ${TESTNAME}")
    list(APPEND DISABLED_TESTS ${TESTNAME})
  endforeach()
endmacro()

#
# Tags tests as experimental. This will add all the --enable=<feature>
# options for the tagged tests.
#
macro(experimental_tests)
  foreach (TESTNAME ${ARGN})
#    message("Marking as experimental ${TESTNAME}")
    list(APPEND EXPERIMENTAL_TESTS ${TESTNAME})
  endforeach()
endmacro()

#
# Tags the given tests as belonging to the given CONFIG, i.e. will
# only be executed when run using ctest -C <CONFIG>
#
# Usage example: set_test_config(Heavy dumptest_testname opencsgtest_testname2)
#
function(set_test_config CONFIG)
  list(APPEND ${CONFIG}_TEST_CONFIG ${ARGN})
  list(FIND TEST_CONFIGS ${CONFIG} FOUND)
  if (FOUND EQUAL -1)
    list(APPEND TEST_CONFIGS ${CONFIG})
    # Export to parent scope
    set(TEST_CONFIGS ${TEST_CONFIGS} PARENT_SCOPE)
  endif()
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} PARENT_SCOPE)
endfunction()

#
# Returns a list of test configs 
#
function(get_test_config TESTNAME CONFIGS)
  foreach(CONFIG ${TEST_CONFIGS})
    list(FIND ${CONFIG}_TEST_CONFIG ${TESTNAME} IDX)
    if (${IDX} GREATER -1)
      list(APPEND ${CONFIGS} ${CONFIG})
    endif()
  endforeach()
  if (${CONFIGS})
    # Convert to a format understood by add_test()
    string(REPLACE ";" "|" ${${CONFIGS}} ${CONFIGS})
    # Export to parent scope
    set(${CONFIGS} ${${CONFIGS}} PARENT_SCOPE)
  endif()
endfunction()

#
# Returns into the FULLNAME variable the global full test name (identifier) 
# given a test command and source filename
#
function(get_test_fullname TESTCMD FILENAME FULLNAME)
  get_filename_component(TESTCMD_NAME ${TESTCMD} NAME_WE)
  get_filename_component(TESTNAME ${FILENAME} NAME_WE)
  string(REPLACE " " "_" TESTNAME ${TESTNAME}) # Test names cannot include spaces
  set(${FULLNAME} ${TESTCMD_NAME}_${TESTNAME})
  # Export to parent scope
  set(${FULLNAME} ${${FULLNAME}} PARENT_SCOPE)
endfunction()

#
# Check if a test file is a 2D test
#
function(is_2d FULLNAME RESULT)

  list(FIND ALL_2D_FILES ${FULLNAME} IDX)
  if (${IDX} GREATER -1)
    set(${RESULT} 1 PARENT_SCOPE)
  else()
    set(${RESULT} PARENT_SCOPE)
  endif()
endfunction()

#
# This functions adds cmd-line tests given files.
#
# Usage add_cmdline_test(testbasename [EXE <executable>] [ARGS <args to exe>]
#                        [SCRIPT <script>]
#                        [EXPECTEDDIR <shared dir>] SUFFIX <suffix> FILES <test files>)
#
if (NOT PYTHON_EXECUTABLE)
  find_package(PythonInterp)
endif()

function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "EXE;SCRIPT;SUFFIX;EXPECTEDDIR" "FILES;ARGS" ${ARGN})

  # If sharing results with another test, pass on this to the python script
  if (TESTCMD_EXPECTEDDIR)
    set(EXTRA_OPTIONS -e ${TESTCMD_EXPECTEDDIR})
  endif()

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CMAKE_BINARY_DIR}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")
    list(FIND DISABLED_TESTS ${TEST_FULLNAME} DISABLED)

    if (${DISABLED} EQUAL -1)

      list(FIND EXPERIMENTAL_TESTS ${TEST_FULLNAME} EXPERIMENTAL)

      if (${EXPERIMENTAL} EQUAL -1)
        set(EXPERIMENTAL_OPTION "")
      else()
        set(EXPERIMENTAL_OPTION "--enable=lc-each" "--enable=lc-else" "--enable=lc-for-c")
      endif()

      # 2D tests should be viewed from the top, not an angle.
      set(CAMERA_OPTION "")
      is_2d(${SCADFILE} IS2D)
      if (IS2D)
        set(CAMERA_OPTION "--camera=0,0,100,0,0,0" "--viewall" "--autocenter" "--projection=ortho")
      endif()

      # Handle configurations
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      if (NOT FOUNDCONFIGS)
        set_test_config(Default ${TEST_FULLNAME})
      endif()
      set_test_config(All ${TEST_FULLNAME})
      list(FIND FOUNDCONFIGS Bugs FOUND)
      if (FOUND EQUAL -1)
        set_test_config(Good ${TEST_FULLNAME})
      endif()

      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      # The python script cannot extract the testname when given extra parameters
      if (TESTCMD_ARGS)
        set(FILENAME_OPTION -f ${FILE_BASENAME})
      endif()

      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${TESTTOOLS_DIR}/test_cmdline_tool.py --comparator=${COMPARATOR} -c ${IMAGE_COMPARE_EXECUTABLE} -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION} ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION} ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS})
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
    endif()
  endforeach()
endfunction()

#
# Usage add_failing_test(testbasename RETVAL <expected return value>
#                        [EXE <executable>] [SCRIPT <script>] [ARGS <args to exe>]
#                        FILES <test files>)
#
function(add_failing_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "RETVAL;EXE;SCRIPT;" "FILES;ARGS" ${ARGN})

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CMAKE_BINARY_DIR}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")
    list(FIND DISABLED_TESTS ${TEST_FULLNAME} DISABLED)

    if (${DISABLED} EQUAL -1)
      # Handle configurations
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      if (NOT FOUNDCONFIGS)
        set_test_config(Default ${TEST_FULLNAME})
      endif()
      set_test_config(All ${TEST_FULLNAME})
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      # The python script cannot extract the testname when given extra parameters
      if (TESTCMD_ARGS)
        set(FILENAME_OPTION -f ${FILE_BASENAME})
      endif()

      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${TESTCMD_ARGS})
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
    endif()
  endforeach()
endfunction()

enable_testing()



set_directory_properties(PROPERTIES TEST_INCLUDE_FILE "${TESTTOOLS_DIR}/EnforceConfig.cmake")

# Subst files
configure_file(${TESTDATA_DIR}/scad/templates/include-tests-template.scad
               ${TESTDATA_DIR}/scad/misc/include-tests.scad)
configure_file(${TESTDATA_DIR}/scad/templates/use-tests-template.scad
               ${TESTDATA_DIR}/scad/misc/use-tests.scad)
configure_file(${TESTDATA_DIR}/scad/templates/import_stl-tests-template.scad
               ${TESTDATA_DIR}/scad/3D/features/import_stl-tests.scad)
configure_file(${TESTDATA_DIR}/scad/templates/import_dxf-tests-template.scad
               ${TESTDATA_DIR}/scad/2D/features/import_dxf-tests.scad)

# Find all scad files
file(GLOB FEATURES_3D_FILES ${TESTDATA_DIR}/scad/3D/features/*.scad)
file(GLOB FEATURES_2D_FILES ${TESTDATA_DIR}/scad/2D/features/*.scad)
file(GLOB DEPRECATED_3D_FILES ${TESTDATA_DIR}/scad/3D/deprecated/*.scad)
file(GLOB ISSUES_3D_FILES ${TESTDATA_DIR}/scad/3D/issues/*.scad)
file(GLOB SCAD_DXF_FILES ${TESTDATA_DIR}/scad/dxf/*.scad)
file(GLOB FUNCTION_FILES ${TESTDATA_DIR}/scad/functions/*.scad)
file(GLOB_RECURSE EXAMPLE_3D_FILES ${EXAMPLES_DIR}/*.scad)

list(REMOVE_ITEM EXAMPLE_3D_FILES
                  ${EXAMPLES_DIR}/Old/example015.scad
                  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
                  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
                  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
                  ${EXAMPLES_DIR}/Functions/recursion.scad)

list(APPEND EXAMPLE_2D_FILES
                  ${EXAMPLES_DIR}/Old/example015.scad
                  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
                  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
                  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
                  ${EXAMPLES_DIR}/Functions/recursion.scad)

list(APPEND EXAMPLE_FILES ${EXAMPLE_3D_FILES} ${EXAMPLE_2D_FILES})

list(APPEND ECHO_FILES ${FUNCTION_FILES}
            ${TESTDATA_DIR}/scad/3D/features/for-tests.scad
            ${TESTDATA_DIR}/scad/misc/expression-evaluation-tests.scad
            ${TESTDATA_DIR}/scad/misc/echo-tests.scad
            ${TESTDATA_DIR}/scad/misc/escape-test.scad
            ${TESTDATA_DIR}/scad/misc/parser-tests.scad
            ${TESTDATA_DIR}/scad/misc/builtin-tests.scad
            ${TESTDATA_DIR}/scad/misc/dim-all.scad
            ${TESTDATA_DIR}/scad/misc/string-test.scad
            ${TESTDATA_DIR}/scad/misc/string-indexing.scad
            ${TESTDATA_DIR}/scad/misc/string-unicode.scad
            ${TESTDATA_DIR}/scad/misc/chr-tests.scad
            ${TESTDATA_DIR}/scad/misc/vector-values.scad
            ${TESTDATA_DIR}/scad/misc/search-tests.scad
            ${TESTDATA_DIR}/scad/misc/search-tests-unicode.scad
            ${TESTDATA_DIR}/scad/misc/recursion-test-function.scad
            ${TESTDATA_DIR}/scad/misc/recursion-test-function2.scad
            ${TESTDATA_DIR}/scad/misc/recursion-test-module.scad
            ${TESTDATA_DIR}/scad/misc/recursion-test-vector.scad
            ${TESTDATA_DIR}/scad/misc/tail-recursion-tests.scad
            ${TESTDATA_DIR}/scad/misc/value-reassignment-tests.scad
            ${TESTDATA_DIR}/scad/misc/value-reassignment-tests2.scad
            ${TESTDATA_DIR}/scad/misc/variable-scope-tests.scad
            ${TESTDATA_DIR}/scad/misc/scope-assignment-tests.scad
            ${TESTDATA_DIR}/scad/misc/lookup-tests.scad
            ${TESTDATA_DIR}/scad/misc/expression-shortcircuit-tests.scad
            ${TESTDATA_DIR}/scad/misc/parent_module-tests.scad
            ${TESTDATA_DIR}/scad/misc/children-tests.scad
            ${TESTDATA_DIR}/scad/misc/range-tests.scad
            ${TESTDATA_DIR}/scad/misc/no-break-space-test.scad
            ${TESTDATA_DIR}/scad/misc/unicode-tests.scad
            ${TESTDATA_DIR}/scad/misc/utf8-tests.scad
            ${TESTDATA_DIR}/scad/misc/nbsp-utf8-test.scad
            ${TESTDATA_DIR}/scad/misc/nbsp-latin1-test.scad
            ${TESTDATA_DIR}/scad/misc/concat-tests.scad
            ${TESTDATA_DIR}/scad/misc/include-tests.scad
            ${TESTDATA_DIR}/scad/misc/include-recursive-test.scad
            ${TESTDATA_DIR}/scad/misc/operators-tests.scad
            ${TESTDATA_DIR}/scad/issues/issue1472.scad
            ${TESTDATA_DIR}/scad/bugs/empty-stl.scad
            ${TESTDATA_DIR}/scad/issues/issue1516.scad
            ${TESTDATA_DIR}/scad/issues/issue1528.scad
            )

list(APPEND DUMPTEST_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES})
list(APPEND DUMPTEST_FILES ${TESTDATA_DIR}/scad/misc/escape-test.scad
                           ${TESTDATA_DIR}/scad/misc/include-tests.scad
                           ${TESTDATA_DIR}/scad/misc/use-tests.scad
                           ${TESTDATA_DIR}/scad/misc/let-module-tests.scad
                           ${TESTDATA_DIR}/scad/misc/localfiles-test.scad
                           ${TESTDATA_DIR}/scad/misc/localfiles_dir/localfiles-compatibility-test.scad
                           ${TESTDATA_DIR}/scad/misc/allexpressions.scad
                           ${TESTDATA_DIR}/scad/misc/allfunctions.scad
                           ${TESTDATA_DIR}/scad/misc/allmodules.scad)

list(APPEND CGALPNGTEST_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${EXAMPLE_2D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${TESTDATA_DIR}/scad/misc/include-tests.scad
                           ${TESTDATA_DIR}/scad/misc/use-tests.scad
                           ${TESTDATA_DIR}/scad/misc/let-module-tests.scad
                           ${TESTDATA_DIR}/scad/bugs/transform-nan-inf-tests.scad
                           ${TESTDATA_DIR}/scad/misc/localfiles-test.scad
                           ${TESTDATA_DIR}/scad/misc/localfiles_dir/localfiles-compatibility-test.scad
                           ${TESTDATA_DIR}/scad/misc/rotate-empty-bbox.scad
                           ${TESTDATA_DIR}/scad/misc/empty-shape-tests.scad
                           ${TESTDATA_DIR}/scad/misc/null-polygons.scad
                           ${TESTDATA_DIR}/scad/misc/internal-cavity.scad
                           ${TESTDATA_DIR}/scad/misc/internal-cavity-polyhedron.scad
                           ${TESTDATA_DIR}/scad/misc/bad-stl-pcbvicebar.scad
                           ${TESTDATA_DIR}/scad/misc/bad-stl-tardis.scad
                           ${TESTDATA_DIR}/scad/misc/bad-stl-wing.scad
                           ${TESTDATA_DIR}/scad/misc/rotate_extrude-hole.scad)

list(APPEND CGALPNGTEST_FILES ${CGALPNGTEST_2D_FILES} ${CGALPNGTEST_3D_FILES})
list(APPEND OPENCSGTEST_FILES ${CGALPNGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${TESTDATA_DIR}/scad/bugs/intersection-prune-test.scad)
list(APPEND THROWNTOGETHERTEST_FILES ${OPENCSGTEST_FILES})

list(APPEND CGALSTLSANITYTEST_FILES ${TESTDATA_DIR}/scad/misc/normal-nan.scad)

list(APPEND EXPORT_STL_TEST_FILES ${TESTDATA_DIR}/scad/stl/stl-export.scad)

list(APPEND EXPORT3D_CGALCGAL_TEST_FILES ${TESTDATA_DIR}/scad/3D/features/polyhedron-nonplanar-tests.scad
                                ${TESTDATA_DIR}/scad/3D/features/rotate_extrude-tests.scad
                                ${TESTDATA_DIR}/scad/3D/features/union-coincident-test.scad
                                ${TESTDATA_DIR}/scad/3D/features/mirror-tests.scad
                                ${TESTDATA_DIR}/scad/misc/null-polygons.scad
                                ${TESTDATA_DIR}/scad/misc/internal-cavity.scad
                                ${TESTDATA_DIR}/scad/misc/internal-cavity-polyhedron.scad
                                ${TESTDATA_DIR}/scad/misc/bad-stl-pcbvicebar.scad
                                ${TESTDATA_DIR}/scad/misc/bad-stl-tardis.scad
                                ${TESTDATA_DIR}/scad/misc/rotate_extrude-hole.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue904.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1105.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1105d.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1215.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1215c.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1221.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1225.scad)

# Issue #910
set_test_config(Bugs 
                     offcgalpngtest_polyhedron-tests
                     offpngtest_nonmanifold-polyhedron
                     offcgalpngtest_bad-stl-pcbvicebar
                     offcgalpngtest_bad-stl-tardis
                     offpngtest_bad-stl-wing
                     offcgalpngtest_issue1105b
                     offcgalpngtest_issue1105c
                     offcgalpngtest_issue1215b
                     offcgalpngtest_issue1258)

list(APPEND EXPORT3D_CGAL_TEST_FILES 
                                ${TESTDATA_DIR}/scad/3D/features/polyhedron-tests.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1105b.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1105c.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1215b.scad
                                ${TESTDATA_DIR}/scad/3D/issues/issue1258.scad)

list(APPEND EXPORT3D_TEST_FILES 
                                ${TESTDATA_DIR}/scad/misc/nonmanifold-polyhedron.scad
                                ${TESTDATA_DIR}/scad/misc/bad-stl-wing.scad)

# No issue - this was introduced when fixing #1033
set_test_config(Bugs stlpngtest_bad-stl-wing)

disable_tests(
  # These don't output anything
  dxfpngtest_text-empty-tests
  dxfpngtest_nothing-decimal-comma-separated
  dxfpngtest_nullspace-2d
  svgpngtest_text-empty-tests
  svgpngtest_nothing-decimal-comma-separated
  svgpngtest_nullspace-2d

  # Not useful
  throwntogethertest_internal-cavity
  throwntogethertest_internal-cavity-polyhedron
  throwntogethertest_nullspace-difference

  # these take too long, for little relative gain in testing
  stlpngtest_iteration
  offpngtest_iteration
  stlpngtest_fractal
  offpngtest_fractal
  stlpngtest_logo_and_text
  offpngtest_logo_and_text

  # Has floating point rounding issues
  dumptest-examples_module_recursion
)

# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${SCAD_SVG_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES ${FILES_2D})

# FIXME: This test illustrates a weakness in child() combined with modifiers.
# Reenable it when this is improved
disable_tests(opencsgtest_child-background)

# These tests only makes sense in OpenCSG mode
disable_tests(cgalpngtest_child-background
              cgalpngtest_highlight-and-background-modifier
              cgalpngtest_highlight-modifier2
              cgalpngtest_background-modifier2
              cgalpngtest_testcolornames
              csgpngtest_child-background
              csgpngtest_highlight-and-background-modifier
              csgpngtest_highlight-modifier2
              csgpngtest_background-modifier2
              csgpngtest_testcolornames
              throwntogethertest_testcolornames)

# This test won't render anything meaningful in throwntogether mode
disable_tests(throwntogethertest_minkowski3-erosion)

# The inf/nan tests fail when exporting CSG and rendering that output again
# as currently inf/nan is written directly to the CSG file (e.g. r = inf)
# which is not valid or even misleading in case a variable inf exists.
# FIXME: define export behavior for inf/nan when exporting CSG files
# These tests return error code 1.
# FIXME: We should have a way of running these and verify the return code
disable_tests(csgpngtest_primitive-inf-tests
              csgpngtest_transform-nan-inf-tests
              csgpngtest_primitive-inf-tests
              csgpngtest_transform-nan-inf-tests
              # Triggers a floating point accuracy issue causing loaded .csg to
              # render slightly differently
              cgalpngtest_nothing-decimal-comma-separated
              cgalpngtest_import-empty-tests
              cgalpngtest_empty-shape-tests
              csgpngtest_issue1258)

experimental_tests(echotest_list-comprehensions-experimental)

# Test config handling

# Heavy tests are tests taking more than 10 seconds on a development computer
set_test_config(Heavy cgalpngtest_rotate_extrude-tests
                      csgpngtest_rotate_extrude-tests
                      cgalpngtest_for-nested-tests
                      csgpngtest_for-nested-tests
                      cgalpngtest_resize-tests
                      cgalpngtest_fractal
                      csgpngtest_fractal
                      cgalpngtest_iteration
                      csgpngtest_iteration
                      cgalpngtest_linear_extrude-scale-zero-tests
                      csgpngtest_linear_extrude-scale-zero-tests
                      cgalpngtest_sphere-tests
                      csgpngtest_resize-tests
                      csgpngtest_resize-tests
                      stlpngtest_fence
                      stlpngtest_surface
                      stlpngtest_demo_cut
                      stlpngtest_search
                      stlpngtest_rounded_box
                      stlpngtest_difference
                      stlpngtest_translation
                      offpngtest_fence
                      offpngtest_surface
                      offpngtest_demo_cut
                      offpngtest_search
                      offpngtest_rounded_box
                      offpngtest_difference
                      offpngtest_translation
                      cgalstlcgalpngtest_rotate_extrude-tests
                      monotonepngtest_rotate_extrude-tests
                      echotest_tail-recursion-tests
                      cgalstlcgalpngtest_rotate_extrude-tests)

# Bugs

list(APPEND BUGS_FILES ${TESTDATA_DIR}/scad/bugs/issue13.scad
                       ${TESTDATA_DIR}/scad/bugs/issue13b.scad
                       ${TESTDATA_DIR}/scad/bugs/issue13c.scad
                       ${TESTDATA_DIR}/scad/bugs/issue584.scad
                       ${TESTDATA_DIR}/scad/bugs/issue591.scad
                       ${TESTDATA_DIR}/scad/bugs/issue666.scad
                       ${TESTDATA_DIR}/scad/bugs/issue791.scad
                       ${TESTDATA_DIR}/scad/bugs/issue802.scad
                       ${TESTDATA_DIR}/scad/bugs/issue899.scad
                       ${TESTDATA_DIR}/scad/bugs/issue945.scad
                       ${TESTDATA_DIR}/scad/bugs/issue945b.scad
                       ${TESTDATA_DIR}/scad/bugs/issue945c.scad
                       ${TESTDATA_DIR}/scad/bugs/issue945d.scad
                       ${TESTDATA_DIR}/scad/bugs/issue945e.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1223.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1223b.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1246.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1455.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1529.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1580-back-to-back.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1580-import-back-to-back.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1580-back-to-back2.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1580-import-back-to-back2.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1580-zero-area-triangle.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1580-import-zero-area-triangle.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1803.scad
)

# We know that we cannot import weakly manifold files into CGAL, so to make tests easier
# to manage, don't try. Once we improve import, we can reenable this
# Known good manifold files -> EXPORT3D_CGALCGAL_TEST_FILES
# Known weak manifold files -> EXPORT3D_CGAL_TEST_FILES
# Known non-manifold files -> EXPORT3D_TEST_FILES
list(APPEND EXPORT3D_CGALCGAL_TEST_FILES ${BUGS_FILES})
#list(REMOVE_ITEM EXPORT3D_CGALCGAL_TEST_FILES
#)
#list(APPEND EXPORT3D_CGAL_TEST_FILES
#)

# 2D files
list(REMOVE_ITEM EXPORT3D_CGALCGAL_TEST_FILES
                       ${TESTDATA_DIR}/scad/bugs/issue899.scad
                       ${TESTDATA_DIR}/scad/bugs/issue1089.scad)
list(APPEND ALL_2D_FILES ${TESTDATA_DIR}/scad/bugs/issue899.scad)

list(APPEND OPENCSGTEST_FILES ${BUGS_FILES})
list(APPEND CGALPNGTEST_FILES ${BUGS_FILES})
foreach(FILE ${BUGS_FILES})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(csgpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(offpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(monotonepngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(stlpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(stlcgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(cgalstlcgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(offpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(offcgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
endforeach()

# Examples

foreach(FILE ${EXAMPLE_FILES})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(throwntogethertest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(csgpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(monotonepngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(stlpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(stlcgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(cgalstlcgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(offpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
  get_test_fullname(offcgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
endforeach()
foreach(FILE ${EXAMPLE_2D_FILES})
  get_test_fullname(dxfpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Examples ${TEST_FULLNAME})
endforeach()

# Workaround Gallium bugs
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
  message(STATUS "Workaround PPC bug https://bugs.freedesktop.org/show_bug.cgi?id=42540")
  set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};GALLIUM_DRIVER=softpipe")
  set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};DRAW_USE_LLVM=no")
endif()

# Set up custom commands to run before & after Ctest run.
# 1. Start/stop Virtual Framebuffer for linux/bsd. 2. Pretty Print
# Please see the CTestCustom.template file for more info. 

#
# Post-test pretty print
#

add_executable(test_pretty_print test_pretty_print.cc)
file(TO_NATIVE_PATH ${TESTTOOLS_DIR}/test_pretty_print.py PYSRC)
set_target_properties(test_pretty_print PROPERTIES COMPILE_FLAGS
  "-DPYBIN=${PYTHON_EXECUTABLE} -DPYSRC=${PYSRC} -DBUILDDIR=--builddir=\"${CMAKE_CURRENT_BINARY_DIR}\""
)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.template TMP)
string(REPLACE __cmake_current_binary_dir__ ${CMAKE_CURRENT_BINARY_DIR} TMP ${TMP})
string(REPLACE __cmake_current_source_dir__ ${CMAKE_CURRENT_SOURCE_DIR} TMP ${TMP})
string(REPLACE __python__ ${PYTHON_EXECUTABLE} TMP ${TMP})
string(REPLACE __header__ "Generated by cmake from ${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.template" TMP ${TMP})
string(REPLACE __cmake_system_name__ ${CMAKE_SYSTEM_NAME} TMP ${TMP})
string(REPLACE __openscad_binpath__ ${OPENSCAD_BINPATH} TMP ${TMP})

set(OPENSCAD_UPLOAD_TESTS $ENV{OPENSCAD_UPLOAD_TESTS})
set(UPLOADARG "")
if (OPENSCAD_UPLOAD_TESTS)
  set(UPLOADARG "--upload")
endif()
string(REPLACE __openscad_upload_tests__ "${UPLOADARG}" TMP ${TMP})

message(STATUS "creating CTestCustom.cmake")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake ${TMP})

#
# Add tests
#
# Types of tests:
# o echotest: Just record console output
# o dumptest: Export .csg
# o cgalpngtest: Export to PNG using --render
# o opencsgtest: Export to PNG using OpenCSG
# o throwntogethertest: Export to PNG using the Throwntogether renderer
# o csgpngtest: 1) Export to .csg, 2) import .csg and export to PNG (--render)
# o monotonepngtest: Same as cgalpngtest but with the "Monotone" color scheme
# o stlpngtest: Export to STL, Re-import and render to PNG (--render)
# o stlcgalpngtest: Export to STL, Re-import and render to PNG (--render=cgal)
# o offpngtest: Export to OFF, Re-import and render to PNG (--render)
# o offcgalpngtest: Export to STL, Re-import and render to PNG (--render=cgal)
# o dxfpngtest: Export to DXF, Re-import and render to PNG (--render=cgal)
#

add_cmdline_test(moduledumptest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX ast FILES
                 ${TESTDATA_DIR}/scad/misc/allmodules.scad
                 ${TESTDATA_DIR}/scad/misc/allfunctions.scad
                 ${TESTDATA_DIR}/scad/misc/allexpressions.scad)
add_cmdline_test(echotest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX echo FILES ${ECHO_FILES})
add_cmdline_test(dumptest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX csg FILES ${DUMPTEST_FILES})
add_cmdline_test(dumptest-examples EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX csg FILES ${EXAMPLE_FILES})
add_cmdline_test(cgalpngtest EXE ${OPENSCAD_BINPATH} ARGS --render -o SUFFIX png FILES ${CGALPNGTEST_FILES})
add_cmdline_test(opencsgtest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX png FILES ${OPENCSGTEST_FILES})
add_cmdline_test(csgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=csg --render EXPECTEDDIR cgalpngtest SUFFIX png FILES ${CGALPNGTEST_FILES})
add_cmdline_test(throwntogethertest EXE ${OPENSCAD_BINPATH} ARGS --preview=throwntogether -o SUFFIX png FILES ${THROWNTOGETHERTEST_FILES})

# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(cgalstlsanitytest EXE ${TESTTOOLS_DIR}/cgalstlsanitytest SUFFIX txt ARGS ${OPENSCAD_BINPATH} FILES ${CGALSTLSANITYTEST_FILES})
add_cmdline_test(csgtexttest SUFFIX txt FILES
                             ${TESTDATA_DIR}/scad/misc/allexpressions.scad
                             ${TESTDATA_DIR}/scad/misc/allfunctions.scad
                             ${TESTDATA_DIR}/scad/misc/allmodules.scad)
add_cmdline_test(csgtermtest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX term FILES
                             ${TESTDATA_DIR}/scad/misc/allexpressions.scad
                             ${TESTDATA_DIR}/scad/misc/allfunctions.scad
                             ${TESTDATA_DIR}/scad/misc/allmodules.scad)

#
# Export/Import tests
#

add_cmdline_test(monotonepngtest EXE ${OPENSCAD_BINPATH} ARGS --colorscheme=Monotone --render -o SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES} ${EXPORT3D_CGALCGAL_TEST_FILES})

# Disabled for now, needs implementation of #420 to be stable
# add_cmdline_test(stlexport EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX stl FILES ${EXPORT_STL_TEST_FILES})

# stlpngtest: direct STL output, preview rendering
add_cmdline_test(stlpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
# cgalstlpngtest: CGAL STL output, normal rendering
add_cmdline_test(stlcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --require-manifold --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})
# cgalstlcgalpngtest: CGAL STL output, CGAL rendering
add_cmdline_test(cgalstlcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})

add_cmdline_test(offpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(offcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})

add_cmdline_test(dxfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D})

add_cmdline_test(svgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --render=cgal --enable=svg-import EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D})


#
# Failing tests
#
add_failing_test(stlfailedtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX stl FILES ${TESTDATA_DIR}/scad/misc/empty-union.scad)
add_failing_test(offfailedtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${TESTTOOLS_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX off FILES ${TESTDATA_DIR}/scad/misc/empty-union.scad)

#
# Add experimental tests
#

# Tests using the actual OpenSCAD binary

# non-ASCII filenames
add_cmdline_test(openscad-nonascii EXE ${OPENSCAD_BINPATH} ARGS -o 
                 SUFFIX csg 
                 FILES ${TESTDATA_DIR}/scad/misc/sfære.scad)


# Variable override (-D arg)

# FIXME - this breaks on older cmake that is very common 'in the wild' on linux
# Override simple variable
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.10)
add_cmdline_test(openscad-override EXE ${OPENSCAD_BINPATH}
                 ARGS -D a=3$<SEMICOLON> -o
                 SUFFIX echo
                 FILES ${TESTDATA_DIR}/scad/misc/override.scad)
endif()

# Image output parameters
add_cmdline_test(openscad-imgsize EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 100,100 -o 
                 SUFFIX png 
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
add_cmdline_test(openscad-imgstretch EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 500,100 -o 
                 SUFFIX png 
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
add_cmdline_test(openscad-imgstretch2 EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 100,500 -o 
                 SUFFIX png 
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camdist EXE ${OPENSCAD_BINPATH} 
                 ARGS --imgsize=500,500 --camera=0,0,0,90,0,90,200 -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camrot EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,0,0,440,337.5,315,200 -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camtrans EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,200 -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective gimbal cam, viewall
add_cmdline_test(openscad-camtrans-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,6000 --viewall -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-20,90,0,90,220 --projection=o -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Orthographic gimbal cam viewall
add_cmdline_test(openscad-camortho-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-cameye EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=120,80,60,0,0,0 -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-cameye2 EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=160,140,130,0,0,0 -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-camcenter EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,60,30,20,10,30  -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Perspective vector cam viewall
add_cmdline_test(openscad-camcenter-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=60,40,30,20,10,30 --viewall -o
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=90,80,75,0,0,0 --projection=o -o 
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)
# Orthographic vector cam viewall
add_cmdline_test(openscad-cameyeortho-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --projection=o -o 
                 SUFFIX png
		 FILES ${TESTDATA_DIR}/scad/3D/features/camera-tests.scad)

# Colorscheme tests
add_cmdline_test(openscad-colorscheme-cornfield EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Cornfield -o 
                 SUFFIX png 
                 FILES ${EXAMPLES_DIR}/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-metallic EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Metallic -o 
                 SUFFIX png 
                 FILES ${EXAMPLES_DIR}/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-sunset EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Sunset -o 
                 SUFFIX png 
                 FILES ${EXAMPLES_DIR}/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-starnight EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Starnight -o 
                 SUFFIX png 
                 FILES ${EXAMPLES_DIR}/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-monotone EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Monotone -o 
                 SUFFIX png 
                 FILES ${EXAMPLES_DIR}/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-metallic-render EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Metallic --render -o 
                 SUFFIX png 
                 FILES ${EXAMPLES_DIR}/Basics/CSG.scad)


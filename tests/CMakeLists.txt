# instructions - see ../doc/testing.txt

#set(DEBUG_OSCD 1) # print debug info during cmake

cmake_minimum_required(VERSION 3.0)
if(POLICY CMP0017)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()

project(openscad_tests)
enable_testing()

# MCAD
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/MCAD/__init__.py)
  message(FATAL_ERROR "MCAD not found. You can install from the OpenSCAD root as follows: \n  git submodule update --init")
endif()

# Create shortcut variables for common paths to keep line lengths down.
set(CCSD ${CMAKE_CURRENT_SOURCE_DIR})
set(EXAMPLES_DIR        "${CCSD}/../examples")
set(TEST_DATA_DIR       "${CCSD}/data")
set(TEST_SCAD_DIR       "${CCSD}/data/scad")
set(TEST_CUSTOMIZER_DIR "${CCSD}/data/scad/customizer")
set(TEST_PYTHON_DIR     "${CCSD}/data/python")

set(CGALSTLSANITYTEST_PY     "${CCSD}/cgalstlsanitytest.py")
set(EXPORT_IMPORT_PNGTEST_PY "${CCSD}/export_import_pngtest.py")
set(EXPORT_PNGTEST_PY        "${CCSD}/export_pngtest.py")
set(SHOULDFAIL_PY            "${CCSD}/shouldfail.py")
set(TEST_CMDLINE_TOOL_PY     "${CCSD}/test_cmdline_tool.py")

# Image comparison - expected test image vs actual generated image

# Imagemagick
if (SKIP_IMAGEMAGICK)
  if (NOT DIFFPNG)
    # cross-building depends on this
    set(IMAGE_COMPARE_EXECUTABLE "/bin/echo")
  endif()
else()
  find_package(ImageMagick COMPONENTS convert)
  if (ImageMagick_convert_FOUND)
    message(STATUS "ImageMagick convert executable found: " ${ImageMagick_convert_EXECUTABLE})
    set(IMAGE_COMPARE_EXECUTABLE ${ImageMagick_convert_EXECUTABLE})
  else()
    message(STATUS "Couldn't find imagemagick 'convert' program")
    set(IMAGEMAGICK_NOBINARY 1)
    set(DIFFPNG 1)
  endif()
endif()

if (not ${IMAGEMACIK_NOBINARY})
  if ("${ImageMagick_VERSION_STRING}" VERSION_LESS "6.5.9.4")
    message(STATUS "ImageMagick version less than 6.5.9.4, cannot use -morphology comparison")
    message(STATUS "ImageMagick Using older image comparison method")
    set(COMPARATOR "--comparator=old")
  endif()

  execute_process(COMMAND ${IMAGE_COMPARE_EXECUTABLE} --version OUTPUT_VARIABLE IM_OUT)
  if (${IM_OUT} MATCHES "OpenMP")
    # http://www.daniloaz.com/en/617/systems/high-cpu-load-when-converting-images-with-imagemagick
    message(STATUS "ImageMagick: OpenMP bug workaround - setting MAGICK_THREAD_LIMIT=1")
    list(APPEND CTEST_ENVIRONMENT "MAGICK_THREAD_LIMIT=1")
  endif()

  message(STATUS "Comparing magicktest1.png with magicktest2.png")
  set(IM_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/magicktest1.png" "${CMAKE_CURRENT_SOURCE_DIR}/magicktest2.png")
  set(COMPARE_ARGS ${IMAGE_COMPARE_EXECUTABLE} ${IM_TEST_FILES} -alpha On -compose difference -composite -threshold 10% -morphology Erode Square -format %[fx:w*h*mean] info:)
  # compare arguments taken from test_cmdline_tool.py
  message(STATUS "Running ImageMagick compare: ${COMPARE_ARGS}")
  execute_process(COMMAND ${COMPARE_ARGS} RESULT_VARIABLE IM_RESULT OUTPUT_VARIABLE IM_OUT)
  message(STATUS "Result: ${IM_RESULT}")
  if (NOT ${IM_RESULT} STREQUAL "0")
    message(STATUS "magicktest1.png and magicktest2.png were incorrectly detected as identical")
    message(STATUS "Using alternative image comparison")
    set(DIFFPNG 1)
  endif()
endif()

if (${DIFFPNG})
  set(IMAGE_COMPARE_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/diffpng)
  set(COMPARATOR "--comparator=diffpng")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/ext/lodepng)
  add_executable(diffpng diffpng.cpp ../src/ext/lodepng/lodepng.cpp)
  message(STATUS "using diffpng for image comparison")
endif()

# Search for MCAD in correct place
list(APPEND CTEST_ENVIRONMENT "OPENSCADPATH=${CMAKE_CURRENT_SOURCE_DIR}/../libraries")

# Platform specific settings

#
# GUI binary tests
#
if(EXISTS "$ENV{OPENSCAD_BINARY}")
  set(OPENSCAD_BINPATH "$ENV{OPENSCAD_BINARY}")
elseif(APPLE)
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../OpenSCAD.app/Contents/MacOS/OpenSCAD")
elseif (MINGW_CROSS_ENV_DIR)
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../mingw32/release/openscad.exe")
elseif(WIN32)
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../openscad.exe")
elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../bin/openscad")
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../bin/openscad")
else()
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../openscad")
endif()

#if(EXISTS "${OPENSCAD_BINPATH}")
#  message(STATUS "Found OpenSCAD binary: ${OPENSCAD_BINPATH}")
#else()
#  message(STATUS "Couldn't find the OpenSCAD binary: ${OPENSCAD_BINPATH}")
#  message(FATAL_ERROR "Please build the OpenSCAD binary and place it here: ${OPENSCAD_BINPATH}")
#endif()
list(APPEND CTEST_ENVIRONMENT "OPENSCAD_BINARY=${OPENSCAD_BINPATH}")

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
# Tags the given tests as belonging to the given CONFIG, i.e. will
# only be executed when run using ctest -C <CONFIG>
#
# Usage example: set_test_config(Heavy dumptest_testname opencsgtest_testname2)
#
function(set_test_config CONFIG)
  cmake_parse_arguments(TESTCFG "" "" "FILES;PREFIXES" ${ARGN})
   # Get fullnames for test files
  if (TESTCFG_PREFIXES)
    foreach(PREFIX ${TESTCFG_PREFIXES})
      foreach(FILE ${TESTCFG_FILES})
        get_test_fullname(${PREFIX} ${FILE} TESTCFG_FULLNAME)
        list(APPEND FULLNAMES ${TESTCFG_FULLNAME})
      endforeach()
    endforeach()
  else()
    list(APPEND FULLNAMES ${TESTCFG_FILES})
  endif()
  # Set config on fullnames
  list(APPEND ${CONFIG}_TEST_CONFIG ${FULLNAMES})
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
find_package(PythonInterp 3.4 REQUIRED)
function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "OPENSCAD;STDIO" "EXE;SCRIPT;SUFFIX;KERNEL;EXPECTEDDIR" "FILES;ARGS" ${ARGN})

  set(EXTRA_OPTIONS "")

  # If sharing results with another test, pass on this to the python script
  if (TESTCMD_EXPECTEDDIR)
    list(APPEND EXTRA_OPTIONS -e ${TESTCMD_EXPECTEDDIR})
  endif()

  if (TESTCMD_KERNEL)
    list(APPEND EXTRA_OPTIONS -k ${TESTCMD_KERNEL})
  endif()

  if (TESTCMD_STDIO)
    list(APPEND EXTRA_OPTIONS --stdin --stdout)
  endif()

  if ((TESTCMD_EXE OR TESTCMD_SCRIPT) AND TESTCMD_OPENSCAD)
    message(FATAL_ERROR "add_cmdline_test() does not allow OPENSCAD flag alongside EXE or SCRIPT values")
  endif()

  # python script implies PYTHON_EXECUTABLE
  if (TESTCMD_SCRIPT MATCHES \\.[Pp][Yy]$)
    set(TESTCMD_EXE ${PYTHON_EXECUTABLE})
  endif()
  if (TESTCMD_OPENSCAD)
    set(TESTCMD_EXE ${OPENSCAD_BINPATH})
  endif()

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CMAKE_CURRENT_BINARY_DIR}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")
    list(FIND DISABLED_TESTS ${TEST_FULLNAME} DISABLED)

    if (${DISABLED} EQUAL -1)

      list(FIND EXPERIMENTAL_TESTS ${TEST_FULLNAME} EXPERIMENTAL_TEST)

      if (${EXPERIMENTAL_TEST} EQUAL -1)
        set(EXPERIMENTAL_OPTION "")
      else()
        # add global experimental options here
        set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=roof" "--enable=lazy-union" "--enable=textmetrics" "--enable=import-function")
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
        set_test_config(Default FILES ${TEST_FULLNAME})
      endif()
      set_test_config(All FILES ${TEST_FULLNAME})
      list(FIND FOUNDCONFIGS Bugs FOUND)
      if (FOUND EQUAL -1)
        set_test_config(Good FILES ${TEST_FULLNAME})
      endif()

      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      # The python script cannot extract the testname when given extra parameters
      #if (TESTCMD_ARGS)
        set(FILENAME_OPTION -f ${FILE_BASENAME})
      #endif()

      # Apply lazy-union to *all* tests for comprehensive testing of this experimental feature.
      # Would need all passing before making lazy-union non-experimental, but that's probably a long way off.
      # Mostly just breaks issues that export non-manifold/intersecting geometry without explicit union.
      #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=lazy-union")

      # Enable vertex-object-renderers by default for all test if experimental build and openscad executable
      if (EXPERIMENTAL AND TESTCMD_OPENSCAD)
        set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers")
        #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-prealloc")
        #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-direct")
        #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-indexing")
      endif()

      # debug message
      #message(STATUS "${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXECUTABLE} -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION} ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION} ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS}")
      # only add test if it is not experimental or if it is and experimental option is enabled
      if (${EXPERIMENTAL_TEST} EQUAL -1 OR EXPERIMENTAL)
        add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXECUTABLE} -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION} ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION} ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS})
        set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT ${CTEST_ENVIRONMENT})
      endif()
    endif()
  endforeach()
endfunction()

#
# Usage add_failing_test(testbasename  RETVAL <expected return value>  SUFFIX <suffix>  FILES <test files>
#                        [EXE <executable>] [SCRIPT <script>] [ARGS <args to exe>])
#
function(add_failing_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "RETVAL;EXE;SCRIPT;SUFFIX" "FILES;ARGS" ${ARGN})

  if ("${TESTCMD_SUFFIX}" STREQUAL "") # Suffix "off" counts as a falsy value, so check diectly for empty string.
    message(FATAL_ERROR "add_failing_test() requires SUFFIX to be set" )
  endif()
  if (NOT TESTCMD_EXE)
    set(TESTCMD_EXE ${PYTHON_EXECUTABLE})
  endif()
  if (NOT TESTCMD_SCRIPT)
    set(TESTCMD_SCRIPT ${SHOULDFAIL_PY})
  endif()

  set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})

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
        set_test_config(Default FILES ${TEST_FULLNAME})
      endif()
      set_test_config(All FILES ${TEST_FULLNAME})
      unset(FOUNDCONFIGS)
      get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
      set(CONFARG CONFIGURATIONS)
      set(CONFVAL ${FOUNDCONFIGS})

      # The python script cannot extract the testname when given extra parameters
      if (TESTCMD_ARGS)
        set(FILENAME_OPTION -f ${FILE_BASENAME})
      endif()

      add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" -s ${TESTCMD_SUFFIX} ${TESTCMD_ARGS})
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
    endif()
  endforeach()
endfunction()


set_directory_properties(PROPERTIES TEST_INCLUDE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/EnforceConfig.cmake")

# Subst files
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

# Find all scad files
file(GLOB DEPRECATED_3D_FILES ${TEST_SCAD_DIR}/3D/deprecated/*.scad)
file(GLOB FEATURES_2D_FILES   ${TEST_SCAD_DIR}/2D/features/*.scad)
file(GLOB FEATURES_3D_FILES   ${TEST_SCAD_DIR}/3D/features/*.scad)
file(GLOB ISSUES_2D_FILES     ${TEST_SCAD_DIR}/2D/issues/*.scad)
file(GLOB ISSUES_3D_FILES     ${TEST_SCAD_DIR}/3D/issues/*.scad)
file(GLOB SCAD_DXF_FILES      ${TEST_SCAD_DIR}/dxf/*.scad)
file(GLOB SCAD_PDF_FILES      ${TEST_SCAD_DIR}/pdf/*.scad)
file(GLOB SCAD_SVG_FILES      ${TEST_SCAD_DIR}/svg/svg-spec/*.scad
  ${TEST_SCAD_DIR}/svg/box-w-holes-2d.scad
  ${TEST_SCAD_DIR}/svg/line-cap-line-join.scad
  ${TEST_SCAD_DIR}/svg/simple-center-2d.scad)
file(GLOB SCAD_AMF_FILES           ${TEST_SCAD_DIR}/amf/*.scad)
file(GLOB SCAD_NEF3_FILES          ${TEST_SCAD_DIR}/nef3/*.scad)
file(GLOB FUNCTION_FILES           ${TEST_SCAD_DIR}/functions/*.scad)
file(GLOB REDEFINITION_FILES       ${TEST_SCAD_DIR}/redefinition/*.scad)
file(GLOB_RECURSE BUGS_FILES       ${TEST_SCAD_DIR}/bugs/*.scad)
file(GLOB_RECURSE BUGS_2D_FILES    ${TEST_SCAD_DIR}/bugs2D/*.scad)
file(GLOB_RECURSE EXAMPLE_3D_FILES ${EXAMPLES_DIR}/*.scad)

list(REMOVE_ITEM EXAMPLE_3D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

list(APPEND EXAMPLE_2D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

list(APPEND EXAMPLE_FILES ${EXAMPLE_3D_FILES} ${EXAMPLE_2D_FILES})

list(APPEND MISC_FILES
  ${TEST_SCAD_DIR}/misc/arg-permutations.scad
  ${TEST_SCAD_DIR}/misc/escape-test.scad
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/include-overwrite-main.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
  ${TEST_SCAD_DIR}/misc/special-consts.scad
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
  ${TEST_SCAD_DIR}/misc/recursion-test-vector.scad
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
  ${TEST_SCAD_DIR}/misc/isobject-test.scad
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
  ${TEST_SCAD_DIR}/misc/text-metrics-test.scad
  ${TEST_SCAD_DIR}/json/import-json.scad
  ${TEST_SCAD_DIR}/misc/data-render.scad
)

list(APPEND ASTDUMPTEST_FILES ${MISC_FILES}
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
)

list(APPEND DUMPTEST_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES})

list(APPEND CGALPNGTEST_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${FEATURES_3D_FILES} ${SCAD_AMF_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES} ${SCAD_NEF3_FILES})
list(APPEND CGALPNGTEST_3D_FILES
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
  ${TEST_SCAD_DIR}/misc/empty-shape-tests.scad
  ${TEST_SCAD_DIR}/misc/null-polygons.scad
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
)

list(APPEND CGALPNGTEST_FILES ${CGALPNGTEST_2D_FILES} ${CGALPNGTEST_3D_FILES})
list(GET CGALPNGTEST_FILES 0 1 2 CGALPNGSTDIOTEST_FILES)
list(APPEND OPENCSGTEST_FILES ${CGALPNGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${TEST_SCAD_DIR}/misc/intersection-prune-test.scad)
list(APPEND THROWNTOGETHERTEST_FILES ${OPENCSGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${STL_IMPORT_FILES})

list(APPEND CGALSTLSANITYTEST_FILES ${TEST_SCAD_DIR}/misc/normal-nan.scad)

list(APPEND EXPORT_STL_TEST_FILES ${TEST_SCAD_DIR}/stl/stl-export.scad)

list(APPEND EXPORT_3MF_TEST_FILES ${TEST_SCAD_DIR}/3mf/3mf-export.scad)

list(APPEND EXPORT3D_CGALCGAL_TEST_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-nonplanar-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-tests.scad
  ${TEST_SCAD_DIR}/3D/features/union-coincident-test.scad
  ${TEST_SCAD_DIR}/3D/features/mirror-tests.scad
  ${TEST_SCAD_DIR}/misc/null-polygons.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/3D/issues/issue904.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1221.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1225.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
  ${TEST_SCAD_DIR}/3D/issues/fn_bug.scad
)

set_test_config(Bugs FILES
  offcgalpngtest_polyhedron-tests
  offpngtest_nonmanifold-polyhedron
  offpngtest_bad-stl-wing

  cgalpngtest_escape-test.scad
)

list(APPEND EXPORT3D_CGAL_TEST_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1258.scad
  ${TEST_SCAD_DIR}/3D/issues/issue2259.scad
)

list(APPEND EXPORT3D_TEST_FILES
  ${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
)

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

  # z-fighting different on different machines
  throwntogethertest_issue1803
  opencsgtest_issue1165
  opencsgtest_issue1215
  throwntogethertest_issue1089
  throwntogethertest_issue1215

  # FIXME: This test illustrates a weakness in child() combined with modifiers.
  # Reenable it when this is improved
  opencsgtest_child-background

  # These tests only makes sense in OpenCSG mode
  cgalpngtest_child-background
  cgalpngtest_highlight-and-background-modifier
  cgalpngtest_highlight-modifier2
  cgalpngtest_background-modifier2
  cgalpngtest_testcolornames
  csgpngtest_child-background
  csgpngtest_highlight-and-background-modifier
  csgpngtest_highlight-modifier2
  csgpngtest_background-modifier2
  csgpngtest_testcolornames
  throwntogethertest_testcolornames

  # This test won't render anything meaningful in throwntogether mode
  throwntogethertest_minkowski3-erosion

  # The inf/nan tests fail when exporting CSG and rendering that output again
  # as currently inf/nan is written directly to the CSG file (e.g. r = inf)
  # which is not valid or even misleading in case a variable inf exists.
  # FIXME: define export behavior for inf/nan when exporting CSG files
  # These tests return error code 1.
  # FIXME: We should have a way of running these and verify the return code
  csgpngtest_primitive-inf-tests
  csgpngtest_transform-nan-inf-tests
  csgpngtest_primitive-inf-tests
  csgpngtest_transform-nan-inf-tests
  # Triggers a floating point accuracy issue causing loaded .csg to
  # render slightly differently
  cgalpngtest_nothing-decimal-comma-separated
  cgalpngtest_import-empty-tests
  cgalpngtest_empty-shape-tests
  csgpngtest_issue1258

)

# Disable LIB3MF tests if library was disabled in build
if(NOT LIB3MF_FOUND)
  # check for package again in case this is qmake build ctest
  if(NOT MSVC)
    pkg_check_modules(LIB3MF lib3MF)
  endif()
  if (NOT LIB3MF_FOUND)
    disable_tests(
      opencsgtest_import_3mf-tests
      cgalpngtest_import_3mf-tests
      csgpngtest_import_3mf-tests
      throwntogethertest_import_3mf-tests
      3mfpngtest_cube10
      3mfexport_3mf-export
    )
  endif()
endif()

# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES ${FILES_2D} ${SCAD_DXF_FILES} ${SCAD_SVG_FILES})


#
# Add experimental features tests
#
experimental_tests(dumptest-examples_roof)
experimental_tests(echotest_allexpressions)
experimental_tests(astdumptest_allexpressions)
experimental_tests(echotest_function-literal-tests)
experimental_tests(echotest_function-literal-compare)
experimental_tests(echotest_isobject-test)
experimental_tests(echotest_text-metrics-test)
experimental_tests(dumptest_text-metrics)
experimental_tests(cgalpngtest_text-metrics)
experimental_tests(opencsgtest_text-metrics)
experimental_tests(csgpngtest_text-metrics)
experimental_tests(throwntogethertest_text-metrics)
experimental_tests(dxfpngtest_text-metrics)
experimental_tests(svgpngtest_text-metrics)
experimental_tests(echotest_import-json)

# Test config handling

# Heavy tests are tests taking more than 10 seconds on a development computer
set_test_config(Heavy FILES
  cgalbinstlcgalpngtest_rotate_extrude-tests
  cgalpngtest_camera-tests
  cgalpngtest_for-nested-tests
  cgalpngtest_fractal
  cgalpngtest_issue267-normalization-crash
  cgalpngtest_iteration
  cgalpngtest_linear_extrude-scale-zero-tests
  cgalpngtest_minkowski3-erosion
  cgalpngtest_projection-extrude-tests
  cgalpngtest_resize-tests
  cgalpngtest_rotate_extrude-angle
  cgalpngtest_rotate_extrude-tests
  cgalpngtest_sphere-tests
  cgalpngtest_surface-tests
  cgalstlcgalpngtest_rotate_extrude-tests
  csgpngtest_camera-tests
  csgpngtest_for-nested-tests
  csgpngtest_fractal
  csgpngtest_issue267-normalization-crash
  csgpngtest_iteration
  csgpngtest_linear_extrude-scale-zero-tests
  csgpngtest_minkowski3-erosion
  csgpngtest_resize-tests
  csgpngtest_rotate_extrude-angle
  csgpngtest_rotate_extrude-tests
  csgpngtest_sphere-tests
  csgpngtest_surface-tests
  monotonepngtest_rotate_extrude-tests
  offpngtest_demo_cut
  offpngtest_difference
  offpngtest_fence
  offpngtest_rounded_box
  offpngtest_search
  offpngtest_surface
  offpngtest_translation
  opencsgtest_issue267-normalization-crash
  opencsgtest_minkowski3-erosion
  openscad-colorscheme-metallic-render_CSG
  stlpngtest_demo_cut
  stlpngtest_difference
  stlpngtest_fence
  stlpngtest_search
  stlpngtest_surface
  stlpngtest_rounded_box
  stlpngtest_translation
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

list(APPEND OPENCSGTEST_FILES ${BUGS_FILES} ${BUGS_2D_FILES})
list(APPEND CGALPNGTEST_FILES ${BUGS_FILES} ${BUGS_2D_FILES})

set_test_config(Bugs FILES ${BUGS_FILES} ${BUGS_2D_FILES} PREFIXES opencsgtest cgalpngtest csgpngtest)
set_test_config(Bugs FILES ${BUGS_FILES} PREFIXES offpngtest monotonepngtest stlpngtest stlcgalpngtest cgalstlcgalpngtest cgalbinstlcgalpngtest offcgalpngtest)

  # Examples
set_test_config(Examples FILES ${EXAMPLE_FILES} PREFIXES cgalpngtest opencsgtest throwntogethertest csgpngtest monotonepngtest stlpngtest stlcgalpngtest cgalstlcgalpngtest cgalbinstlcgalpngtest offpngtest offcgalpngtest)
set_test_config(Examples FILES ${EXAMPLE_2D_FILES} PREFIXES dxfpngtest)

# Workaround Gallium bugs
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
  message(STATUS "Workaround PPC bug https://bugs.freedesktop.org/show_bug.cgi?id=42540")
  list(APPEND CTEST_ENVIRONMENT "GALLIUM_DRIVER=softpipe")
  list(APPEND CTEST_ENVIRONMENT "DRAW_USE_LLVM=no")
endif()
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
  message(STATUS "Workaround MIPS bug https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=868745")
  list(APPEND CTEST_ENVIRONMENT "GALLIUM_DRIVER=softpipe")
  list(APPEND CTEST_ENVIRONMENT "DRAW_USE_LLVM=no")
endif()

# Set up custom commands to run before & after Ctest run.
# 1. Start/stop Virtual Framebuffer for linux/bsd. 2. Pretty Print
# Please see the CTestCustom.template file for more info.

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

add_cmdline_test(astdumptest      OPENSCAD SUFFIX ast FILES ${ASTDUMPTEST_FILES})
add_cmdline_test(astdumpstdiotest OPENSCAD SUFFIX ast FILES ${TEST_SCAD_DIR}/misc/allexpressions.scad STDIO EXPECTEDDIR astdumptest ARGS --export-format ast)

add_cmdline_test(csgtermtest      OPENSCAD SUFFIX term FILES
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
)
add_cmdline_test(echotest         OPENSCAD SUFFIX echo FILES ${ECHO_FILES})
add_cmdline_test(echostdiotest    OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/echo-tests.scad STDIO EXPECTEDDIR echotest ARGS --export-format echo)
add_cmdline_test(echotest         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/builtin-invalid-range-test.scad ARGS --check-parameter-ranges=on)

# generate a very large scad file which we would rather not commit to the source tree
# this is for stress-testing the parser
add_custom_target(issue2342 ALL
  COMMAND ${PYTHON_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_issue2342.py ">${TEST_SCAD_DIR}/issues/issue2342.scad"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating issue2342.scad"
)
add_custom_target(svg_viewbox_tests ALL
  COMMAND ${PYTHON_EXECUTABLE} ${TEST_PYTHON_DIR}/gen_svg_viewbox_tests.py "${TEST_DATA_DIR}/svg/viewbox" "${TEST_SCAD_DIR}/svg/extruded"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating svg viewbox tests"
)

add_cmdline_test(dumptest           OPENSCAD FILES ${DUMPTEST_FILES} SUFFIX csg ARGS)
add_cmdline_test(dumptest-examples  OPENSCAD FILES ${EXAMPLE_FILES} SUFFIX csg ARGS)
add_cmdline_test(cgalpngtest        OPENSCAD FILES ${CGALPNGTEST_FILES} SUFFIX png ARGS --render)
add_cmdline_test(cgalpngstdiotest   OPENSCAD FILES ${CGALPNGSTDIOTEST_FILES} SUFFIX png STDIO EXPECTEDDIR cgalpngtest ARGS --export-format png --render)
add_cmdline_test(opencsgtest        OPENSCAD FILES ${OPENCSGTEST_FILES} SUFFIX png ARGS)
add_cmdline_test(csgpngtest         SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${CGALPNGTEST_FILES} EXPECTEDDIR cgalpngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=csg --render)
add_cmdline_test(throwntogethertest OPENSCAD FILES ${THROWNTOGETHERTEST_FILES} ARGS --preview=throwntogether SUFFIX png)
# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(cgalstlsanitytest  SCRIPT ${CGALSTLSANITYTEST_PY} SUFFIX txt FILES ${CGALSTLSANITYTEST_FILES} ARGS ${OPENSCAD_BINPATH})

list(APPEND SVG_VIEWBOX_TESTS
  viewbox_300x400_none viewbox_600x200_none
  viewbox_300x400_meet_xMinYMin viewbox_300x400_meet_xMidYMin viewbox_300x400_meet_xMaxYMin
  viewbox_600x200_meet_xMinYMin viewbox_600x200_meet_xMinYMid viewbox_600x200_meet_xMinYMax
  viewbox_600x200_slice_xMinYMin viewbox_600x200_slice_xMidYMin viewbox_600x200_slice_xMaxYMin
  viewbox_600x600_slice_xMinYMin viewbox_600x600_slice_xMinYMid viewbox_600x600_slice_xMinYMax
)

foreach(TEST ${SVG_VIEWBOX_TESTS})
  add_cmdline_test(svgviewbox-${TEST} OPENSCAD ARGS --imgsize 600,600 "-Dfile=\"../../../svg/viewbox/${TEST}.svg\";" SUFFIX png FILES ${TEST_SCAD_DIR}/svg/extruded/viewbox-test.scad)
endforeach()

add_cmdline_test(svgimport OPENSCAD ARGS --imgsize 600,600 SUFFIX png FILES
  ${TEST_SCAD_DIR}/svg/extruded/box-w-holes.scad
  ${TEST_SCAD_DIR}/svg/extruded/simple-center.scad
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

experimental_tests(lazyunion-dump_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-dump_lazyunion-toplevel-objects)
experimental_tests(lazyunion-dump_lazyunion-toplevel-for)
experimental_tests(lazyunion-dump_lazyunion-nested-for)
experimental_tests(lazyunion-dump_lazyunion-children)
experimental_tests(lazyunion-dump_lazyunion-hull-for)
experimental_tests(lazyunion-dump_lazyunion-root-for)
experimental_tests(lazyunion-dump_lazyunion-intersection-for)
experimental_tests(lazyunion-dump_lazyunion-difference-for)
experimental_tests(lazyunion-dump_lazyunion-minkowski-for)
experimental_tests(lazyunion-dump_lazyunion-transform-for)
experimental_tests(lazyunion-dump_2d-3d)
experimental_tests(lazyunion-opencsg_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-opencsg_lazyunion-toplevel-objects)
experimental_tests(lazyunion-opencsg_lazyunion-toplevel-for)
experimental_tests(lazyunion-opencsg_lazyunion-nested-for)
experimental_tests(lazyunion-opencsg_lazyunion-children)
experimental_tests(lazyunion-opencsg_lazyunion-hull-for)
experimental_tests(lazyunion-opencsg_lazyunion-root-for)
experimental_tests(lazyunion-opencsg_lazyunion-intersection-for)
experimental_tests(lazyunion-opencsg_lazyunion-difference-for)
experimental_tests(lazyunion-opencsg_lazyunion-minkowski-for)
experimental_tests(lazyunion-opencsg_lazyunion-transform-for)
experimental_tests(lazyunion-opencsg_2d-3d)
experimental_tests(lazyunion-cgalpng_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-cgalpng_lazyunion-toplevel-objects)
experimental_tests(lazyunion-cgalpng_lazyunion-toplevel-for)
experimental_tests(lazyunion-cgalpng_lazyunion-nested-for)
experimental_tests(lazyunion-cgalpng_lazyunion-children)
experimental_tests(lazyunion-cgalpng_lazyunion-hull-for)
experimental_tests(lazyunion-cgalpng_lazyunion-root-for)
experimental_tests(lazyunion-cgalpng_lazyunion-intersection-for)
experimental_tests(lazyunion-cgalpng_lazyunion-difference-for)
experimental_tests(lazyunion-cgalpng_lazyunion-minkowski-for)
experimental_tests(lazyunion-cgalpng_lazyunion-transform-for)
experimental_tests(lazyunion-cgalpng_2d-3d)
experimental_tests(lazyunion-monotonepng_lazyunion-toplevel-objects)
experimental_tests(lazyunion-monotonepng_lazyunion-toplevel-for)
experimental_tests(lazyunion-monotonepng_lazyunion-nested-for)
experimental_tests(lazyunion-monotonepng_lazyunion-children)
experimental_tests(lazyunion-monotonepng_lazyunion-hull-for)
experimental_tests(lazyunion-monotonepng_lazyunion-root-for)
experimental_tests(lazyunion-monotonepng_lazyunion-intersection-for)
experimental_tests(lazyunion-monotonepng_lazyunion-difference-for)
experimental_tests(lazyunion-monotonepng_lazyunion-minkowski-for)
experimental_tests(lazyunion-monotonepng_lazyunion-transform-for)
experimental_tests(lazyunion-monotonepng_2d-3d)
experimental_tests(lazyunion-stlpngtest_lazyunion-toplevel-objects)
experimental_tests(lazyunion-stlpngtest_lazyunion-toplevel-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-nested-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-children)
experimental_tests(lazyunion-stlpngtest_lazyunion-hull-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-root-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-intersection-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-difference-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-minkowski-for)
experimental_tests(lazyunion-stlpngtest_lazyunion-transform-for)
experimental_tests(lazyunion-stlpngtest_2d-3d)
experimental_tests(lazyunion-offpngtest_lazyunion-toplevel-objects)
experimental_tests(lazyunion-offpngtest_lazyunion-toplevel-for)
experimental_tests(lazyunion-offpngtest_lazyunion-nested-for)
experimental_tests(lazyunion-offpngtest_lazyunion-children)
experimental_tests(lazyunion-offpngtest_lazyunion-hull-for)
experimental_tests(lazyunion-offpngtest_lazyunion-root-for)
experimental_tests(lazyunion-offpngtest_lazyunion-intersection-for)
experimental_tests(lazyunion-offpngtest_lazyunion-difference-for)
experimental_tests(lazyunion-offpngtest_lazyunion-minkowski-for)
experimental_tests(lazyunion-offpngtest_lazyunion-transform-for)
experimental_tests(lazyunion-offpngtest_2d-3d)
experimental_tests(lazyunion-dxfpngtest_lazyunion-toplevel-2dobjects)
experimental_tests(lazyunion-svgpngtest_lazyunion-toplevel-2dobjects)

add_cmdline_test(lazyunion-dump        OPENSCAD SUFFIX csg FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-opencsg     OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-cgalpng     OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union --render)
add_cmdline_test(lazyunion-monotonepng OPENSCAD SUFFIX png FILES ${LAZYUNION_3D_FILES} ARGS --colorscheme=Monotone --enable=lazy-union --render )
add_cmdline_test(lazyunion-stlpngtest  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} EXPECTEDDIR lazyunion-monotonepng ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --enable=lazy-union --render=cgal)
add_cmdline_test(lazyunion-offpngtest  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} EXPECTEDDIR lazyunion-monotonepng ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --enable=lazy-union --render=cgal)
add_cmdline_test(lazyunion-dxfpngtest  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-cgalpng     ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --enable=lazy-union --render=cgal)
add_cmdline_test(lazyunion-svgpngtest  SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-cgalpng     ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --enable=lazy-union --render=cgal)

#
# Trivial Export/Import files
# This sanity-checks bidirectional file format import/export
#

list(APPEND TRIVIAL_IMPORT_EXPORT_2D_FILES ${TEST_SCAD_DIR}/misc/square10.scad)
list(APPEND TRIVIAL_IMPORT_EXPORT_3D_FILES ${TEST_SCAD_DIR}/misc/cube10.scad)

add_cmdline_test(monotonepngtest OPENSCAD SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_2D_FILES} ${TRIVIAL_IMPORT_EXPORT_3D_FILES} ARGS --colorscheme=Monotone --render)
add_cmdline_test(stlpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES} EXPECTEDDIR monotonepngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=STL)
add_cmdline_test(offpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES} EXPECTEDDIR monotonepngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF)
add_cmdline_test(amfpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES} EXPECTEDDIR monotonepngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=AMF)
add_cmdline_test(3mfpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES} EXPECTEDDIR monotonepngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=3MF)
add_cmdline_test(dxfpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_2D_FILES} EXPECTEDDIR monotonepngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --render=cgal)
add_cmdline_test(svgpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_2D_FILES} EXPECTEDDIR monotonepngtest ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --render=cgal)
add_cmdline_test(pdfexporttest SCRIPT ${EXPORT_PNGTEST_PY} SUFFIX png FILES ${SCAD_PDF_FILES} EXPECTEDDIR pdfexporttest ARGS --openscad=${OPENSCAD_BINPATH} --format=PDF KERNEL Square:2)

#
# Corner-case Export/Import tests
#

add_cmdline_test(monotonepngtest OPENSCAD SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES} ${EXPORT3D_CGALCGAL_TEST_FILES} ARGS --colorscheme=Monotone --render)

# Disabled for now, needs implementation of #420 to be stable
#add_cmdline_test(stlexport             ARGS SUFFIX stl FILES ${EXPORT_STL_TEST_FILES})
add_cmdline_test(3mfexport             OPENSCAD ARGS SUFFIX 3mf FILES ${EXPORT_3MF_TEST_FILES})

# stlpngtest: direct STL output, preview rendering
add_cmdline_test(stlpngtest            SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=STL EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
# cgalstlpngtest: CGAL STL output, normal rendering
add_cmdline_test(stlcgalpngtest        SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --require-manifold --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})
# cgalstlcgalpngtest: CGAL STL output, CGAL rendering
add_cmdline_test(cgalstlcgalpngtest    SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=ASCIISTL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})

# cgalbinstlcgalpngtest: CGAL binary STL output, CGAL rendering
add_cmdline_test(cgalbinstlcgalpngtest SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=BINSTL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})
add_cmdline_test(offpngtest            SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(offcgalpngtest        SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})
add_cmdline_test(dxfpngtest            SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D} ${SCAD_DXF_FILES})
add_cmdline_test(svgpngtest            SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D} ${SCAD_SVG_FILES})

# Failing tests
add_failing_test(stlfailedtest         SUFFIX stl  FILES ${TEST_SCAD_DIR}/misc/empty-union.scad ARGS --retval=1)
add_failing_test(offfailedtest         SUFFIX off  FILES ${TEST_SCAD_DIR}/misc/empty-union.scad ARGS --retval=1)
add_failing_test(parsererrors          SUFFIX stl  FILES ${FAILING_FILES} ARGS --retval=1)
# Hardwarning Test
add_failing_test(hardwarnings          SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/errors-warnings.scad ARGS --retval=1 --hardwarnings)

# Verify that test framework is paying attention to alpha channel, issue 1492
#add_cmdline_test(openscad-colorscheme-cornfield-alphafail  ARGS --colorscheme=Cornfield SUFFIX png FILES ${EXAMPLES_DIR}/Basics/logo.scad)

# The "expected image" supplied for this "alphafail" test has the alpha channel for all background pixels cleared (a==0), when they should be opaque (a==1) for this colorscheme.  so if test framework is functioning properly then the image comparison should fail
# Commented out because the master branch isn't capable of making the expected image yet. Also TEST_GENERATE=1 makes an expected image that makes the test fail.
#set_property(TEST openscad-colorscheme-cornfield-alphafail_logo PROPERTY WILL_FAIL TRUE)


#
# Customizer tests
#
add_cmdline_test(customizertest OPENSCAD ARGS SUFFIX ast FILES
  ${TEST_CUSTOMIZER_DIR}/description.scad
  ${TEST_CUSTOMIZER_DIR}/parameter.scad
  ${TEST_CUSTOMIZER_DIR}/allmodulescomment.scad
  ${TEST_CUSTOMIZER_DIR}/allfunctionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/allexpressionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/group.scad
)

add_cmdline_test(customizertest-first          OPENSCAD FILES ${TEST_CUSTOMIZER_DIR}/setofparameter.scad SUFFIX ast ARGS -p ${TEST_CUSTOMIZER_DIR}/setofparameter.json -P firstSet)
add_cmdline_test(customizertest-wrong          OPENSCAD FILES ${TEST_CUSTOMIZER_DIR}/setofparameter.scad SUFFIX ast ARGS -p ${TEST_CUSTOMIZER_DIR}/setofparameter.json -P wrongSetValues)
add_cmdline_test(customizertest-incomplete     OPENSCAD FILES ${TEST_CUSTOMIZER_DIR}/setofparameter.scad SUFFIX ast ARGS -p ${TEST_CUSTOMIZER_DIR}/setofparameter.json -P thirdSet)
add_cmdline_test(customizertest-imgset         OPENSCAD FILES ${TEST_CUSTOMIZER_DIR}/setofparameter.scad SUFFIX ast ARGS -p ${TEST_CUSTOMIZER_DIR}/setofparameter.json -P imagine)
add_cmdline_test(customizertest-setNameWithDot OPENSCAD FILES ${TEST_CUSTOMIZER_DIR}/setofparameter.scad SUFFIX ast ARGS -p ${TEST_CUSTOMIZER_DIR}/setofparameter.json -P Name.dot)

# non-ASCII filenames
add_cmdline_test(openscad-nonascii             OPENSCAD FILES ${TEST_SCAD_DIR}/misc/sfære.scad SUFFIX csg)

# Variable override (-D arg)

# FIXME - this breaks on older cmake that is very common 'in the wild' on linux
# Override simple variable
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.10)
add_cmdline_test(openscad-override         OPENSCAD FILES ${TEST_SCAD_DIR}/misc/override.scad SUFFIX echo ARGS -D a=3$<SEMICOLON>)
endif()

# Image output parameters
add_cmdline_test(openscad-imgsize          OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize 100,100)
add_cmdline_test(openscad-imgstretch       OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize 500,100)
add_cmdline_test(openscad-imgstretch2      OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize 100,500)
# Perspective gimbal cam
add_cmdline_test(openscad-camdist          OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,0,0,90,0,90,200)
# Perspective gimbal cam
add_cmdline_test(openscad-camrot           OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,0,0,440,337.5,315,200)
# Perspective gimbal cam
add_cmdline_test(openscad-camtrans         OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,200)
# Perspective gimbal cam, viewall
add_cmdline_test(openscad-camtrans-viewall OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,6000 --viewall)
# Perspective gimbal cam, viewall, autocenter, off-center
add_cmdline_test(openscad-camtrans-viewall-offcenter OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests-offcenter.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,0,0,30,40,50,10 --viewall --autocenter)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho         OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=100,-20,-20,90,0,90,220 --projection=o)
# Orthographic gimbal cam viewall
add_cmdline_test(openscad-camortho-viewall OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o)
# Perspective vector cam
add_cmdline_test(openscad-cameye           OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=120,80,60,0,0,0)
add_cmdline_test(openscad-cameye_front     OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,-130,0,0,0,0)
add_cmdline_test(openscad-cameye_back      OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,130,0,0,0,0)
add_cmdline_test(openscad-cameye_left      OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=-130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_right     OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_top       OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,0,130,0,0,0)
add_cmdline_test(openscad-cameye_bottom    OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=0,0,-130,0,0,0)

# Perspective vector cam
add_cmdline_test(openscad-cameye2             OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=160,140,130,0,0,0)
# Perspective vector cam
add_cmdline_test(openscad-camcenter           OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=100,60,30,20,10,30)
# Perspective vector cam viewall
add_cmdline_test(openscad-camcenter-viewall   OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=60,40,30,20,10,30 --viewall)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho         OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=90,80,75,0,0,0 --projection=o)
# Orthographic vector cam viewall
add_cmdline_test(openscad-cameyeortho-viewall OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --projection=o)

add_cmdline_test(openscad-camvp-variables     OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-vp.scad SUFFIX png ARGS --imgsize=500,500)
add_cmdline_test(openscad-camvp-override      OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/camera-vp.scad SUFFIX png ARGS --imgsize=500,500 --camera=120,80,60,0,0,0)

# View Options tests
add_cmdline_test(openscad-viewoptions-axes              OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view axes)
add_cmdline_test(openscad-viewoptions-axes-scales       OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view axes,scales)
add_cmdline_test(openscad-viewoptions-edges             OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view edges)
add_cmdline_test(openscad-viewoptions-axes-scales-edges OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view axes,scales,edges)
add_cmdline_test(openscad-viewoptions-wireframe         OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --render --view wireframe)
add_cmdline_test(openscad-viewoptions-crosshairs        OPENSCAD FILES ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad SUFFIX png ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --render --view crosshairs)

# Colorscheme tests
add_cmdline_test(openscad-colorscheme-cornfield       OPENSCAD FILES ${EXAMPLES_DIR}/Basics/logo.scad SUFFIX png ARGS --colorscheme=Cornfield)
add_cmdline_test(openscad-colorscheme-metallic        OPENSCAD FILES ${EXAMPLES_DIR}/Basics/logo.scad SUFFIX png ARGS --colorscheme=Metallic)
add_cmdline_test(openscad-colorscheme-sunset          OPENSCAD FILES ${EXAMPLES_DIR}/Basics/logo.scad SUFFIX png ARGS --colorscheme=Sunset)
add_cmdline_test(openscad-colorscheme-starnight       OPENSCAD FILES ${EXAMPLES_DIR}/Basics/logo.scad SUFFIX png ARGS --colorscheme=Starnight)
add_cmdline_test(openscad-colorscheme-monotone        OPENSCAD FILES ${EXAMPLES_DIR}/Basics/logo.scad SUFFIX png ARGS --colorscheme=Monotone)
add_cmdline_test(openscad-colorscheme-metallic-render OPENSCAD FILES ${EXAMPLES_DIR}/Basics/CSG.scad  SUFFIX png ARGS --colorscheme=Metallic --render)

#message("Available test configurations: ${TEST_CONFIGS}")
#foreach(CONF ${TEST_CONFIGS})
#  message("${CONF}: ${${CONF}_TEST_CONFIG}")
#endforeach()

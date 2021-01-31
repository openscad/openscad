# instructions - see ../doc/testing.txt

#set(DEBUG_OSCD 1) # print debug info during cmake

cmake_minimum_required(VERSION 3.0)
if(POLICY CMP0017)
  # Explicitly use new include policy to avoid globally shadowing included modules
  # http://www.cmake.org/cmake/help/cmake-2-8-docs.html#policy:CMP0017
  cmake_policy(SET CMP0017 NEW)
endif()

project(tests)

# MCAD
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/MCAD/__init__.py)
  message(FATAL_ERROR "MCAD not found. You can install from the OpenSCAD root as follows: \n  git submodule update --init")
endif()

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

if ( not ${IMAGEMACIK_NOBINARY} )
  if ( "${ImageMagick_VERSION_STRING}" VERSION_LESS "6.5.9.4" )
    message(STATUS "ImageMagick version less than 6.5.9.4, cannot use -morphology comparison")
    message(STATUS "ImageMagick Using older image comparison method")
    set(COMPARATOR "old")
  endif()

  execute_process(COMMAND ${IMAGE_COMPARE_EXECUTABLE} --version OUTPUT_VARIABLE IM_OUT )
  if ( ${IM_OUT} MATCHES "OpenMP" )
    # http://www.daniloaz.com/en/617/systems/high-cpu-load-when-converting-images-with-imagemagick
    message(STATUS "ImageMagick: OpenMP bug workaround - setting MAGICK_THREAD_LIMIT=1")
    set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};MAGICK_THREAD_LIMIT=1")
  endif()

  message(STATUS "Comparing magicktest1.png with magicktest2.png")
  set(IM_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/magicktest1.png" "${CMAKE_CURRENT_SOURCE_DIR}/magicktest2.png")
  set(COMPARE_ARGS ${IMAGE_COMPARE_EXECUTABLE} ${IM_TEST_FILES} -alpha On -compose difference -composite -threshold 10% -morphology Erode Square -format %[fx:w*h*mean] info:)
  # compare arguments taken from test_cmdline_tool.py
  message(STATUS "Running ImageMagick compare: ${COMPARE_ARGS}")
  execute_process(COMMAND ${COMPARE_ARGS} RESULT_VARIABLE IM_RESULT OUTPUT_VARIABLE IM_OUT )
  message(STATUS "Result: ${IM_RESULT}")
  if ( NOT ${IM_RESULT} STREQUAL "0" )
    message(STATUS "magicktest1.png and magicktest2.png were incorrectly detected as identical")
    message(STATUS "Using alternative image comparison")
    set(DIFFPNG 1)
  endif()
endif()

if ( ${DIFFPNG} )
  set(IMAGE_COMPARE_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/diffpng)
  set(COMPARATOR "diffpng")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/ext/lodepng)
  add_executable(diffpng diffpng.cpp ../src/ext/lodepng/lodepng.cpp)
  message(STATUS "using diffpng for image comparison")
endif()

# Search for MCAD in correct place
set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};OPENSCADPATH=${CMAKE_CURRENT_SOURCE_DIR}/../libraries")

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
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../Release/openscad.exe")
elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/../bin/openscad")
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../bin/openscad")
else()
  set(OPENSCAD_BINPATH "${CMAKE_CURRENT_BINARY_DIR}/../openscad")
endif()

#if(EXISTS "${OPENSCAD_BINPATH}")
#  message(STATUS "Found OpenSCAD binary: ${OPENSCAD_BINPATH}")
#else()
#  message(STATUS "Couldn't find the OpenSCAD binary: ${OPENSCAD_BINPATH}")
#  message(FATAL_ERROR "Please build the OpenSCAD binary and place it here: ${OPENSCAD_BINPATH}" )
#endif()

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
find_package(PythonInterp 3.4 REQUIRED)
function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "EXE;SCRIPT;SUFFIX;KERNEL;EXPECTEDDIR;STDIN;STDOUT" "FILES;ARGS" ${ARGN})

  set(EXTRA_OPTIONS "")

  # If sharing results with another test, pass on this to the python script
  if (TESTCMD_EXPECTEDDIR)
    list(APPEND EXTRA_OPTIONS -e ${TESTCMD_EXPECTEDDIR})
  endif()

  if (TESTCMD_KERNEL)
    list(APPEND EXTRA_OPTIONS -k ${TESTCMD_KERNEL})
  endif()

  if (TESTCMD_STDIN)
    list(APPEND EXTRA_OPTIONS --stdin)
  endif()

  if (TESTCMD_STDOUT)
    list(APPEND EXTRA_OPTIONS --stdout)
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
        set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=lazy-union")
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

      # Apply lazy-union to *all* tests for comprehensive testing of this experimental feature.
      # Would need all passing before making lazy-union non-experimental, but that's probably a long way off.
      # Mostly just breaks issues that export non-manifold/intersecting geometry without explicit union.
      #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=lazy-union")

      # Enable vertex-object-renderers by default for all test if experimental build and openscad executable
      if (EXPERIMENTAL)
        if (${TESTCMD_EXE} MATCHES "^.*openscad$")
          set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers")
          #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-prealloc")
          #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-direct")
          #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-indexing")
        endif()
      endif()

      # debug message
      #message("${TEST_FULLNAME} ${CONFVAL} ${PYTHON_EXECUTABLE} ${tests_SOURCE_DIR}/test_cmdline_tool.py --comparator=${COMPARATOR} -c ${IMAGE_COMPARE_EXECUTABLE} -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION} ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION} ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS}")
      # only add test if it is not experimental or if it is and experimental option is enabled
      if (${EXPERIMENTAL_TEST} EQUAL -1 OR EXPERIMENTAL)
      	add_test(NAME ${TEST_FULLNAME} ${CONFARG} ${CONFVAL} COMMAND ${PYTHON_EXECUTABLE} ${tests_SOURCE_DIR}/test_cmdline_tool.py --comparator=${COMPARATOR} -c ${IMAGE_COMPARE_EXECUTABLE} -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION} ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION} ${EXPERIMENTAL_OPTION} ${TESTCMD_ARGS})
      	set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
      endif()
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
    set(TESTCMD_EXE ${CMAKE_CURRENT_BINARY_DIR}/${TESTCMD_BASENAME})
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



set_directory_properties(PROPERTIES TEST_INCLUDE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/EnforceConfig.cmake")

# Subst files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/templates/include-tests-template.scad
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/templates/use-tests-template.scad
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/templates/import_stl-tests-template.scad
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/import_stl-tests.scad)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/templates/import_3mf-tests-template.scad
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/import_3mf-tests.scad)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/templates/import_dxf-tests-template.scad
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/2D/features/import_dxf-tests.scad)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/python/gen_issue2342-template.py
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/python/gen_issue2342.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../testdata/python/gen_svg_viewbox_tests-template.py
               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/python/gen_svg_viewbox_tests.py)

# Find all scad files
file(GLOB FEATURES_3D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/*.scad)
file(GLOB FEATURES_2D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/2D/features/*.scad)
file(GLOB DEPRECATED_3D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/deprecated/*.scad)
file(GLOB ISSUES_2D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/2D/issues/*.scad)
file(GLOB ISSUES_3D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/*.scad)
file(GLOB SCAD_DXF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/dxf/*.scad)
file(GLOB SCAD_PDF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/pdf/*.scad)
file(GLOB SCAD_SVG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/svg-spec/*.scad
  ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/box-w-holes-2d.scad
  ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/line-cap-line-join.scad
  ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/simple-center-2d.scad)
file(GLOB SCAD_AMF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/amf/*.scad)
file(GLOB SCAD_NEF3_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/nef3/*.scad)
file(GLOB FUNCTION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/*.scad)
file(GLOB REDEFINITION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/redefinition/*.scad)
file(GLOB_RECURSE EXAMPLE_3D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/*.scad)
file(GLOB_RECURSE BUGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/bugs/*.scad)
file(GLOB_RECURSE BUGS_2D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/bugs2D/*.scad)

list(REMOVE_ITEM EXAMPLE_3D_FILES
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Old/example015.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Advanced/module_recursion.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Functions/list_comprehensions.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Functions/polygon_areas.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Functions/recursion.scad)

list(APPEND EXAMPLE_2D_FILES
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Old/example015.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Advanced/module_recursion.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Functions/list_comprehensions.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Functions/polygon_areas.scad
                  ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Functions/recursion.scad)

list(APPEND EXAMPLE_FILES ${EXAMPLE_3D_FILES} ${EXAMPLE_2D_FILES})

list(APPEND MISC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/escape-test.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/include-overwrite-main.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-tests.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/let-module-tests.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/localfiles-test.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/localfiles_dir/localfiles-compatibility-test.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allfunctions.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allmodules.scad
                       ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/special-consts.scad)


list(APPEND FAILING_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1890-comment.scad
                          ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1890-include.scad
                          ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1890-string.scad
                          ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1890-use.scad
                          )

list(APPEND ECHO_FILES ${FUNCTION_FILES} ${MISC_FILES} ${REDEFINITION_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/for-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/rotate-parameters.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/expression-evaluation-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/echo-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-fail1-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-fail2-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-fail3-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-fail4-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-fail5-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/for-c-style-infinite-loop.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/parser-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/builtin-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/dim-all.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/string-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/string-indexing.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/string-unicode.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/chr-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/ord-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/vector-values.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/search-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/search-tests-unicode.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/recursion-test-function.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/recursion-test-function2.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/recursion-test-function3.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/recursion-test-module.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/recursion-test-vector.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/tail-recursion-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/value-reassignment-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/value-reassignment-tests2.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/variable-scope-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/scope-assignment-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/lookup-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/expression-shortcircuit-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/parent_module-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/children-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/range-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/no-break-space-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/unicode-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/utf8-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/nbsp-utf8-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/nbsp-latin1-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/concat-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/include-recursive-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/errors-warnings.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/errors-warnings-included.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/children-warnings-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/isundef-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/islist-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/isnum-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/isbool-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/isstring-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/operators-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/expression-precedence.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/builtins-calling-vec3vec2.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1472.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/empty-stl.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1516.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1528.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1923.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/preview_variable.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue1851-each-fail-on-scalar.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue2342.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue3118-recur-limit.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue3541.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/function-scope.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/root-modifiers.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/root-modifier-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/use-order-test/use-order-test.scad
            )

list(APPEND ASTDUMPTEST_FILES ${MISC_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/assert-expression-fail1-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/assert-expression-fail2-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/assert-expression-fail3-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/assert-expression-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/echo-expression-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/expression-precedence-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/let-test-single.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/let-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/list-comprehensions.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/functions/exponent-operator-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/ifelse-ast-dump.scad
            )

list(APPEND DUMPTEST_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES})

list(APPEND CGALPNGTEST_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${FEATURES_3D_FILES} ${SCAD_AMF_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES} ${SCAD_NEF3_FILES})
list(APPEND CGALPNGTEST_3D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/include-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/use-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/assert-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/let-module-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/localfiles-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/localfiles_dir/localfiles-compatibility-test.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/rotate-empty-bbox.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/empty-shape-tests.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/null-polygons.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/internal-cavity.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/internal-cavity-polyhedron.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/bad-stl-pcbvicebar.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/bad-stl-tardis.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/bad-stl-wing.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/rotate_extrude-hole.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/preview_variable.scad
            )

# test importing unparseable files, result will be an empty image
list(APPEND STL_IMPORT_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/stl/stl-import-invalidvertex.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/stl/stl-import-toomanyvertices.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/stl/stl-import-unparseable.scad
            )

list(APPEND CGALPNGTEST_FILES ${CGALPNGTEST_2D_FILES} ${CGALPNGTEST_3D_FILES})
list(SUBLIST CGALPNGTEST_FILES 0 1 CGALPNGSTDIOTEST_FILES)
list(APPEND OPENCSGTEST_FILES ${CGALPNGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/intersection-prune-test.scad)
list(APPEND THROWNTOGETHERTEST_FILES ${OPENCSGTEST_FILES})
list(APPEND OPENCSGTEST_FILES ${STL_IMPORT_FILES})

list(APPEND CGALSTLSANITYTEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/normal-nan.scad)

list(APPEND EXPORT_STL_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/stl/stl-export.scad)

list(APPEND EXPORT_3MF_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3mf/3mf-export.scad)

list(APPEND EXPORT3D_CGALCGAL_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/polyhedron-nonplanar-tests.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/rotate_extrude-tests.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/union-coincident-test.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/mirror-tests.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/null-polygons.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/internal-cavity.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/internal-cavity-polyhedron.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/bad-stl-pcbvicebar.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/bad-stl-tardis.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/rotate_extrude-hole.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue904.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1105.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1105d.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1215.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1215c.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1221.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1225.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/preview_variable.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/fn_bug.scad
                                )

set_test_config(Bugs 
  # Issue #910
  offcgalpngtest_polyhedron-tests
  offpngtest_nonmanifold-polyhedron
  offpngtest_bad-stl-wing

  cgalpngtest_escape-test.scad
)

list(APPEND EXPORT3D_CGAL_TEST_FILES 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/polyhedron-tests.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1105b.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1105c.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1215b.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue1258.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/issues/issue2259.scad)

list(APPEND EXPORT3D_TEST_FILES 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/nonmanifold-polyhedron.scad
                                ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/bad-stl-wing.scad)

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
experimental_tests(echotest_allexpressions)
experimental_tests(astdumptest_allexpressions)
experimental_tests(echotest_function-literal-tests)
experimental_tests(echotest_function-literal-compare)

# Test config handling

# Heavy tests are tests taking more than 10 seconds on a development computer
set_test_config(Heavy 
  cgalpngtest_rotate_extrude-tests
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
  csgpngtest_sphere-tests
  csgpngtest_resize-tests
  csgpngtest_resize-tests
  csgpngtest_minkowski3-erosion
  cgalpngtest_minkowski3-erosion
  opencsgtest_minkowski3-erosion
  cgalpngtest_camera-tests
  csgpngtest_camera-tests
  cgalpngtest_surface-tests
  csgpngtest_surface-tests
  csgpngtest_rotate_extrude-angle
  cgalpngtest_rotate_extrude-angle
  cgalpngtest_projection-extrude-tests
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
  cgalbinstlcgalpngtest_rotate_extrude-tests
  monotonepngtest_rotate_extrude-tests
  openscad-colorscheme-metallic-render_CSG
  cgalpngtest_issue267-normalization-crash
  csgpngtest_issue267-normalization-crash
  opencsgtest_issue267-normalization-crash
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
foreach(FILE ${BUGS_FILES} ${BUGS_2D_FILES})
  get_test_fullname(opencsgtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(cgalpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
  get_test_fullname(csgpngtest ${FILE} TEST_FULLNAME)
  set_test_config(Bugs ${TEST_FULLNAME})
endforeach()
foreach(FILE ${BUGS_FILES})
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
  get_test_fullname(cgalbinstlcgalpngtest ${FILE} TEST_FULLNAME)
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
  get_test_fullname(cgalbinstlcgalpngtest ${FILE} TEST_FULLNAME)
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
if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
  message(STATUS "Workaround MIPS bug https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=868745")
  set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};GALLIUM_DRIVER=softpipe")
  set(CTEST_ENVIRONMENT "${CTEST_ENVIRONMENT};DRAW_USE_LLVM=no")
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

add_cmdline_test(astdumptest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX ast FILES ${ASTDUMPTEST_FILES})
add_cmdline_test(astdumpstdiotest EXE ${OPENSCAD_BINPATH} ARGS --export-format ast -o SUFFIX ast STDOUT true STDIN true EXPECTEDDIR astdumptest FILES
                                  ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad)


add_cmdline_test(csgtermtest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX term FILES
                             ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allexpressions.scad
                             ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allfunctions.scad
                             ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/allmodules.scad)
add_cmdline_test(echotest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX echo FILES ${ECHO_FILES})
add_cmdline_test(echostdiotest EXE ${OPENSCAD_BINPATH} ARGS --export-format echo -o SUFFIX echo STDOUT true STDIN true EXPECTEDDIR echotest FILES
                               ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/echo-tests.scad)
add_cmdline_test(echotest EXE ${OPENSCAD_BINPATH} ARGS --check-parameter-ranges=on -o SUFFIX echo FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/builtin-invalid-range-test.scad)

# generate a very large scad file which we would rather not commit to the source tree
# this is for stress-testing the parser
set(GEN_SCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/python)
add_custom_target(issue2342 ALL 
  COMMAND ${PYTHON_EXECUTABLE} ${GEN_SCRIPT_DIR}/gen_issue2342.py ">${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/issues/issue2342.scad"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating issue2342.scad"
)
add_custom_target(svg_viewbox_tests ALL
  COMMAND ${PYTHON_EXECUTABLE} ${GEN_SCRIPT_DIR}/gen_svg_viewbox_tests.py "${CMAKE_CURRENT_SOURCE_DIR}/../testdata/svg/viewbox" "${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/extruded"
  WORKING_DIRECTORY ${GEN_SCRIPT_DIR}
  COMMENT "Generating svg viewbox tests"
)

add_cmdline_test(dumptest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX csg FILES ${DUMPTEST_FILES})
add_cmdline_test(dumptest-examples EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX csg FILES ${EXAMPLE_FILES})
add_cmdline_test(cgalpngtest EXE ${OPENSCAD_BINPATH} ARGS --render -o SUFFIX png FILES ${CGALPNGTEST_FILES})
add_cmdline_test(cgalpngstdiotest EXE ${OPENSCAD_BINPATH} ARGS --export-format png --render -o SUFFIX png STDIN true STDOUT true EXPECTEDDIR cgalpngtest FILES ${CGALPNGSTDIOTEST_FILES})
add_cmdline_test(opencsgtest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX png FILES ${OPENCSGTEST_FILES})
add_cmdline_test(csgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=csg --render EXPECTEDDIR cgalpngtest SUFFIX png FILES ${CGALPNGTEST_FILES})
add_cmdline_test(throwntogethertest EXE ${OPENSCAD_BINPATH} ARGS --preview=throwntogether -o SUFFIX png FILES ${THROWNTOGETHERTEST_FILES})
# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error
add_cmdline_test(cgalstlsanitytest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cgalstlsanitytest SUFFIX txt ARGS ${OPENSCAD_BINPATH} FILES ${CGALSTLSANITYTEST_FILES})

list(APPEND SVG_VIEWBOX_TESTS
    viewbox_300x400_none viewbox_600x200_none
    viewbox_300x400_meet_xMinYMin viewbox_300x400_meet_xMidYMin viewbox_300x400_meet_xMaxYMin
    viewbox_600x200_meet_xMinYMin viewbox_600x200_meet_xMinYMid viewbox_600x200_meet_xMinYMax
    viewbox_600x200_slice_xMinYMin viewbox_600x200_slice_xMidYMin viewbox_600x200_slice_xMaxYMin
    viewbox_600x600_slice_xMinYMin viewbox_600x600_slice_xMinYMid viewbox_600x600_slice_xMinYMax)

foreach(TEST ${SVG_VIEWBOX_TESTS})
    add_cmdline_test(svgviewbox-${TEST} EXE ${OPENSCAD_BINPATH} ARGS --imgsize 600,600 "-Dfile=\"../../../svg/viewbox/${TEST}.svg\";" -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/extruded/viewbox-test.scad)
endforeach()
add_cmdline_test(svgimport EXE ${OPENSCAD_BINPATH} ARGS --imgsize 600,600 -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/extruded/box-w-holes.scad)
add_cmdline_test(svgimport EXE ${OPENSCAD_BINPATH} ARGS --imgsize 600,600 -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/svg/extruded/simple-center.scad)

# lazy-union
list(APPEND LAZYUNION_3D_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-toplevel-objects.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-toplevel-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-nested-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-children.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-hull-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-root-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-intersection-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-difference-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-minkowski-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-transform-for.scad
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/features/2d-3d.scad)

list(APPEND LAZYUNION_2D_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/experimental/lazyunion-toplevel-2dobjects.scad)

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

add_cmdline_test(lazyunion-dump EXE ${OPENSCAD_BINPATH} ARGS --enable=lazy-union -o SUFFIX csg FILES ${LAZYUNION_FILES})
add_cmdline_test(lazyunion-opencsg EXE ${OPENSCAD_BINPATH} ARGS --enable=lazy-union -o SUFFIX png FILES ${LAZYUNION_FILES})
add_cmdline_test(lazyunion-cgalpng EXE ${OPENSCAD_BINPATH} ARGS --enable=lazy-union --render -o SUFFIX png FILES ${LAZYUNION_FILES})
add_cmdline_test(lazyunion-monotonepng EXE ${OPENSCAD_BINPATH} ARGS --colorscheme=Monotone --enable=lazy-union --render -o SUFFIX png FILES ${LAZYUNION_3D_FILES})
add_cmdline_test(lazyunion-stlpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --enable=lazy-union --render=cgal EXPECTEDDIR lazyunion-monotonepng SUFFIX png FILES ${LAZYUNION_3D_FILES})
add_cmdline_test(lazyunion-offpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --enable=lazy-union --render=cgal EXPECTEDDIR lazyunion-monotonepng SUFFIX png FILES ${LAZYUNION_3D_FILES})
add_cmdline_test(lazyunion-dxfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --enable=lazy-union --render=cgal EXPECTEDDIR lazyunion-cgalpng SUFFIX png FILES ${LAZYUNION_2D_FILES})
add_cmdline_test(lazyunion-svgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --enable=lazy-union --render=cgal EXPECTEDDIR lazyunion-cgalpng SUFFIX png FILES ${LAZYUNION_2D_FILES})

#
# Trivial Export/Import files
# This sanity-checks bidirectional file format import/export
#

list(APPEND TRIVIAL_IMPORT_EXPORT_2D_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/square10.scad)
list(APPEND TRIVIAL_IMPORT_EXPORT_3D_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/cube10.scad)

add_cmdline_test(monotonepngtest EXE ${OPENSCAD_BINPATH} ARGS --colorscheme=Monotone --render -o SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_2D_FILES} ${TRIVIAL_IMPORT_EXPORT_3D_FILES})
add_cmdline_test(stlpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL EXPECTEDDIR monotonepngtest SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES})
add_cmdline_test(offpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF EXPECTEDDIR monotonepngtest SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES})
add_cmdline_test(amfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=AMF EXPECTEDDIR monotonepngtest SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES})
add_cmdline_test(3mfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=3MF EXPECTEDDIR monotonepngtest SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_3D_FILES})
add_cmdline_test(dxfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_2D_FILES})
add_cmdline_test(svgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${TRIVIAL_IMPORT_EXPORT_2D_FILES})
add_cmdline_test(pdfexporttest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=PDF KERNEL Square:2 EXPECTEDDIR pdfexporttest SUFFIX png FILES ${SCAD_PDF_FILES})

#
# Corner-case Export/Import tests
#

add_cmdline_test(monotonepngtest EXE ${OPENSCAD_BINPATH} ARGS --colorscheme=Monotone --render -o SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES} ${EXPORT3D_CGALCGAL_TEST_FILES})

# Disabled for now, needs implementation of #420 to be stable
# add_cmdline_test(stlexport EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX stl FILES ${EXPORT_STL_TEST_FILES})

add_cmdline_test(3mfexport EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX 3mf FILES ${EXPORT_3MF_TEST_FILES})

# stlpngtest: direct STL output, preview rendering
add_cmdline_test(stlpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
# cgalstlpngtest: CGAL STL output, normal rendering
add_cmdline_test(stlcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=STL --require-manifold --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})
# cgalstlcgalpngtest: CGAL STL output, CGAL rendering
add_cmdline_test(cgalstlcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=ASCIISTL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})

# cgalbinstlcgalpngtest: CGAL binary STL output, CGAL rendering
add_cmdline_test(cgalbinstlcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=BINSTL --require-manifold --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGALCGAL_TEST_FILES})

add_cmdline_test(offpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_TEST_FILES})
add_cmdline_test(offcgalpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=OFF --render=cgal EXPECTEDDIR monotonepngtest SUFFIX png FILES ${EXPORT3D_CGAL_TEST_FILES})

add_cmdline_test(dxfpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=DXF --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D} ${SCAD_DXF_FILES})

add_cmdline_test(svgpngtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/export_import_pngtest.py ARGS --openscad=${OPENSCAD_BINPATH} --format=SVG --render=cgal EXPECTEDDIR cgalpngtest SUFFIX png FILES ${FILES_2D} ${SCAD_SVG_FILES})

#
# Failing tests
#
add_failing_test(stlfailedtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX stl FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/empty-union.scad)
add_failing_test(offfailedtest EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX off FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/empty-union.scad)
add_failing_test(parsererrors EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 -o SUFFIX stl FILES ${FAILING_FILES})

# Hardwarning Test       
add_failing_test(hardwarnings EXE ${PYTHON_EXECUTABLE} SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/shouldfail.py ARGS --openscad=${OPENSCAD_BINPATH} --retval=1 --hardwarnings -o SUFFIX echo FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/errors-warnings.scad)

# Verify that test framework is paying attention to alpha channel, issue 1492
#add_cmdline_test(openscad-colorscheme-cornfield-alphafail EXE ${OPENSCAD_BINPATH} ARGS --colorscheme=Cornfield -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/logo.scad)

# The "expected image" supplied for this "alphafail" test has the alpha channel for all background pixels cleared (a==0), when they should be opaque (a==1) for this colorscheme.  so if test framework is functioning properly then the image comparison should fail
# Commented out because the master branch isn't capable of making the expected image yet. Also TEST_GENERATE=1 makes an expected image that makes the test fail.
#set_property(TEST openscad-colorscheme-cornfield-alphafail_logo PROPERTY WILL_FAIL TRUE)


#
# Customizer tests
#
add_cmdline_test(customizertest EXE ${OPENSCAD_BINPATH} ARGS -o SUFFIX ast FILES
                 ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/description.scad
                 ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/parameter.scad
                 ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/allmodulescomment.scad
                 ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/allfunctionscomment.scad
                 ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/allexpressionscomment.scad
                 ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/group.scad
                 )

add_cmdline_test(customizertest-first EXE ${OPENSCAD_BINPATH} ARGS -p ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.json -P firstSet -o SUFFIX ast FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.scad)
add_cmdline_test(customizertest-wrong EXE ${OPENSCAD_BINPATH} ARGS -p ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.json -P wrongSetValues -o SUFFIX ast FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.scad)
add_cmdline_test(customizertest-incomplete EXE ${OPENSCAD_BINPATH} ARGS -p ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.json -P thirdSet -o SUFFIX ast FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.scad)
add_cmdline_test(customizertest-imgset EXE ${OPENSCAD_BINPATH} ARGS -p ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.json -P imagine -o SUFFIX ast FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.scad)
add_cmdline_test(customizertest-setNameWithDot EXE ${OPENSCAD_BINPATH} ARGS -p ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.json -P Name.dot -o SUFFIX ast FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/customizer/setofparameter.scad)
# Tests using the actual OpenSCAD binary

# non-ASCII filenames
add_cmdline_test(openscad-nonascii EXE ${OPENSCAD_BINPATH} ARGS -o 
                 SUFFIX csg 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/sfære.scad)

# Variable override (-D arg)

# FIXME - this breaks on older cmake that is very common 'in the wild' on linux
# Override simple variable
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_GREATER 2.8.10)
add_cmdline_test(openscad-override EXE ${OPENSCAD_BINPATH}
                 ARGS -D a=3$<SEMICOLON> -o
                 SUFFIX echo
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/misc/override.scad)
endif()

# Image output parameters
add_cmdline_test(openscad-imgsize EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 100,100 -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-imgstretch EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 500,100 -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-imgstretch2 EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize 100,500 -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camdist EXE ${OPENSCAD_BINPATH} 
                 ARGS --imgsize=500,500 --camera=0,0,0,90,0,90,200 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camrot EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,0,0,440,337.5,315,200 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective gimbal cam
add_cmdline_test(openscad-camtrans EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,200 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective gimbal cam, viewall
add_cmdline_test(openscad-camtrans-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,6000 --viewall -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective gimbal cam, viewall, autocenter, off-center
add_cmdline_test(openscad-camtrans-viewall-offcenter EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,0,0,30,40,50,10 --viewall --autocenter -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests-offcenter.scad)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-20,90,0,90,220 --projection=o -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Orthographic gimbal cam viewall
add_cmdline_test(openscad-camortho-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-cameye EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=120,80,60,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-cameye_front EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,-130,0,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-cameye_back EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,130,0,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-cameye_left EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=-130,0,0,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-cameye_right EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=130,0,0,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-cameye_top EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,0,130,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
add_cmdline_test(openscad-cameye_bottom EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=0,0,-130,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)

# Perspective vector cam
add_cmdline_test(openscad-cameye2 EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=160,140,130,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective vector cam
add_cmdline_test(openscad-camcenter EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=100,60,30,20,10,30  -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Perspective vector cam viewall
add_cmdline_test(openscad-camcenter-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=60,40,30,20,10,30 --viewall -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=90,80,75,0,0,0 --projection=o -o 
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)
# Orthographic vector cam viewall
add_cmdline_test(openscad-cameyeortho-viewall EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --projection=o -o 
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-tests.scad)

add_cmdline_test(openscad-camvp-variables EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-vp.scad)
add_cmdline_test(openscad-camvp-override EXE ${OPENSCAD_BINPATH}
                 ARGS --imgsize=500,500 --camera=120,80,60,0,0,0 -o
                 SUFFIX png
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/camera-vp.scad)

# View Options tests
add_cmdline_test(openscad-viewoptions-axes EXE ${OPENSCAD_BINPATH} ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view axes -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/view-options-tests.scad)
add_cmdline_test(openscad-viewoptions-axes-scales EXE ${OPENSCAD_BINPATH} ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view axes,scales -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/view-options-tests.scad)
add_cmdline_test(openscad-viewoptions-edges EXE ${OPENSCAD_BINPATH} ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view edges -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/view-options-tests.scad)
add_cmdline_test(openscad-viewoptions-axes-scales-edges EXE ${OPENSCAD_BINPATH} ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --view axes,scales,edges -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/view-options-tests.scad)
add_cmdline_test(openscad-viewoptions-wireframe EXE ${OPENSCAD_BINPATH} ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --render --view wireframe -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/view-options-tests.scad)
add_cmdline_test(openscad-viewoptions-crosshairs EXE ${OPENSCAD_BINPATH} ARGS --imgsize=500,500 --camera=16,14,13,0,0,0 --viewall --render --view crosshairs -o SUFFIX png FILES ${CMAKE_CURRENT_SOURCE_DIR}/../testdata/scad/3D/misc/view-options-tests.scad)

# Colorscheme tests
add_cmdline_test(openscad-colorscheme-cornfield EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Cornfield -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-metallic EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Metallic -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-sunset EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Sunset -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-starnight EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Starnight -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-monotone EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Monotone -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/logo.scad)
add_cmdline_test(openscad-colorscheme-metallic-render EXE ${OPENSCAD_BINPATH}
                 ARGS --colorscheme=Metallic --render -o 
                 SUFFIX png 
                 FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/Basics/CSG.scad)

#message("Available test configurations: ${TEST_CONFIGS}")
#foreach(CONF ${TEST_CONFIGS})
#  message("${CONF}: ${${CONF}_TEST_CONFIG}")
#endforeach()

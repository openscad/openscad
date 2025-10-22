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
# Usage example: set_test_config(Heavy dump_testname preview_testname2)
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
    list(SORT TEST_CONFIGS)
    # Export to parent scope
    set(TEST_CONFIGS ${TEST_CONFIGS} CACHE INTERNAL "")
  endif()
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} CACHE INTERNAL "")
endfunction(set_test_config)

#
# Removes a tag from the given tests
#
# Usage example: remove_test_config(Bugs FILES preview_testname2)
#
function(remove_test_config CONFIG)
  cmake_parse_arguments(TESTCFG "" "" "FILES" ${ARGN})
  list(APPEND FULLNAMES ${TESTCFG_FILES})
  # Remove config from fullnames
  list(REMOVE_ITEM ${CONFIG}_TEST_CONFIG ${FULLNAMES})
  # Export to parent scope
  set(${CONFIG}_TEST_CONFIG ${${CONFIG}_TEST_CONFIG} CACHE INTERNAL "")
endfunction(remove_test_config)

#
# Returns a list of test configs
#
function(get_test_config TESTNAME OUTVAR)
  unset(CONFIGS)
  foreach(CONFIG ${TEST_CONFIGS})
    list(FIND ${CONFIG}_TEST_CONFIG ${TESTNAME} IDX)
    if (IDX GREATER -1)
      list(APPEND CONFIGS ${CONFIG})
    endif()
  endforeach()
  set(${OUTVAR} "${CONFIGS}" PARENT_SCOPE)
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

#[=[
.. function:: regex_alt(OUT OLD NEW)

  Merge function that combines two regex fragments using a non-capturing
  alternation: ``(?:OLD)|(?:NEW)``. Handles empty sides.

  Synopsis
    regex_alt(<out-var> <old> <new>)

  Arguments
    - OUT : Name of the variable to receive the merged pattern.
    - OLD : Existing regex fragment (may be empty).
    - NEW : Regex fragment to add.

  Behaviour
    - If only one side is non-empty, returns that side unchanged.
    - If both are non-empty, returns ``(?:OLD)|(?:NEW)`` to avoid changing
      precedence.
    - Writes the result to ``OUT`` (parent scope).

  Notes
    - Pass raw fragments (no surrounding ``^…$`` unless you intend to anchor).
    - Prefer bracket-quoted arguments (e.g., ``[[...]]``) to avoid CMake
      escape processing and list splitting.

  Example
    regex_alt(_p [[^TRACE:.*\(.*\)$]] [[^\s*\*\*\* Excluding \d+ frames \*\*\*\s*$]])
    # _p now holds a safe alternation of the two patterns.
]=]
function(regex_alt OUT OLD NEW)
  if(OLD STREQUAL "")
    set(res "${NEW}")
  elseif(NEW STREQUAL "")
    set(res "${OLD}")
  else()
    set(res "(?:${OLD})|(?:${NEW})")
  endif()
  set(${OUT} "${res}" PARENT_SCOPE)
endfunction()

#[=[
.. function:: str_concat(OUT OLD NEW)

  Merge function that concatenates ``OLD`` and ``NEW`` strings.

  Synopsis
    str_concat(<out-var> <old> <new>)

  Arguments
    - OUT : Name of the variable to receive the concatenated string.
    - OLD : Existing value (may be empty).
    - NEW : Value to append.

  Behaviour
    Writes ``OLD`` followed by ``NEW`` into ``OUT`` (parent scope). Useful as
    the ``MERGE_FN`` for ``append_to_value`` / ``test_env_append_value`` when
    simple string concatenation (not regex alternation) is desired.

  Example
    # FOO_FLAGS := (existing) + ":-extra"
    test_env_append_value(some_test FOO_FLAGS ":-extra" str_concat)
]=]
function(str_concat OUT OLD NEW)
  set(${OUT} "${OLD}${NEW}" PARENT_SCOPE)
endfunction()

#[=[
.. function:: append_to_value(OUT KEY VALUE KEY_VALUES MERGE_FN)

  Pure helper that merges ``VALUE`` into the ``KEY`` entry within a list of
  ``KEY=VALUE`` pairs and returns the updated list via ``OUT``.

  Synopsis
    append_to_value(<out-var> <key> <value> <key-values> <merge-fn>)

  Arguments
    - OUT        : Name of the variable to receive the updated list.
    - KEY        : Entry name to modify.
    - VALUE      : New fragment to merge into the existing value for ``KEY``.
    - KEY_VALUES : CMake list of ``KEY=VALUE`` strings (e.g., a test's
                   ``ENVIRONMENT`` property).
    - MERGE_FN   : Merge function name with signature ``(OUT, OLD, NEW)``.

  Behaviour
    Locates the last occurrence of ``KEY=...`` in ``KEY_VALUES``, extracts
    ``OLD``, computes the merged result by calling ``MERGE_FN``, escapes
    semicolons in the merged value, and produces a new list with a single
    ``KEY=merged`` entry plus all other untouched entries. The new list is
    written to ``OUT`` (parent scope).

  Notes
    - Use bracket quotes for regex values and quote arguments when calling to
      avoid list splitting.

  Example
    get_property(_env TEST echo_sample PROPERTY ENVIRONMENT)
    append_to_value(_env2 OPENSCAD_TEST_EXCLUDE_LINE
                    [[^TRACE:.*\(.*\)$]] "${_env}" regex_alt)
    set_property(TEST echo_sample PROPERTY ENVIRONMENT "${_env2}")
]=]
function(append_to_value OUT KEY VALUE KEY_VALUES MERGE_FN)
  if(NOT COMMAND ${MERGE_FN})
    message(FATAL_ERROR "append_to_value: unknown merge fn '${MERGE_FN}'")
  endif()

  # Find existing KEY and keep other entries
  set(_others)
  set(_old "")
  foreach(kv IN LISTS KEY_VALUES)
    if(kv MATCHES "^${KEY}=")
      string(REGEX REPLACE "^${KEY}=" "" _old "${kv}") # keep last seen
    else()
      list(APPEND _others "${kv}")
    endif()
  endforeach()

  # Compute merged value
  cmake_language(CALL ${MERGE_FN} _merged "${_old}" "${VALUE}")

  # Protect semicolons (list delimiter)
  string(REPLACE ";" "\\;" _merged_esc "${_merged}")

  # Build output list
  list(APPEND _others "${KEY}=${_merged_esc}")
  set(${OUT} "${_others}" PARENT_SCOPE)
endfunction()

#[=[
.. function:: ctest_env_append_value(KEY VALUE [MERGE_FN])

  Merge ``VALUE`` into the ``KEY`` entry of the global ``CTEST_ENVIRONMENT``
  list, producing the baseline environment that will be copied to **every**
  test created thereafter.

  Synopsis
    ctest_env_append_value(<key> <value> [<merge-fn>])

  Arguments
    - KEY       : Environment variable name to modify (e.g., ``OPENSCADPATH``,
                  ``OPENSCAD_TEST_EXCLUDE_LINE``).
    - VALUE     : String to merge into the existing value for ``KEY``. Quote
                  it; for regex, prefer bracket quotes like ``[[...]]``.
    - MERGE_FN  : Optional merge function name (default: ``regex_alt``) with
                  signature ``(OUT, OLD, NEW)``. Common choices:
                  ``regex_alt`` (non-capturing alternation) or
                  ``str_concat`` (plain concatenation).

  Behaviour
    - Reads the current ``CTEST_ENVIRONMENT`` (a CMake list of ``KEY=VALUE``
      strings) from the caller's scope.
    - Uses ``append_to_value`` with ``MERGE_FN`` to combine the existing value
      for ``KEY`` (if any) with ``VALUE``.
    - Escapes semicolons in the merged value and writes the updated list back
      to ``CTEST_ENVIRONMENT`` (parent scope).
    - If ``KEY`` was not present, a new ``KEY=VALUE`` entry is appended.
      If multiple entries existed, they are collapsed into a single merged
      entry at the end (CTest uses the **last** assignment for a given key).

  Notes
    - Call this **before** invoking functions that create tests (e.g.,
      ``add_test(...)`` / ``add_cmdline_test(...)``) so all tests inherit the
      baseline. For one-off, per-test changes use
      ``test_env_append_value(TEST_NAME KEY VALUE MERGE_FN)`` instead.
    - Keep regex/text values quoted to avoid list splitting; bracket quotes
      like ``[[...]]`` are convenient for regex.

  Examples
    # Set common paths for all tests
    ctest_env_append_value(OPENSCADPATH    "${LIBRARIES_DIR}" str_concat)
    ctest_env_append_value(OPENSCAD_BINARY "${OPENSCAD_BINPATH}" str_concat)

    # Provide a base ignore pattern for all text comparisons (regex OR)
    ctest_env_append_value(OPENSCAD_TEST_EXCLUDE_LINE
      [[^TRACE:.*\((?:\d+)\s+frames\s+(?:omitted|skipped)\).*$]]
      regex_alt)
]=]
function(ctest_env_append_value KEY VALUE)
  # default merger if not provided
  set(MERGE_FN regex_alt)
  if(ARGC GREATER 2)
    set(MERGE_FN "${ARGV2}")
  endif()

  # Current baseline (may be empty)
  set(_env "${CTEST_ENVIRONMENT}")

  # Use your existing pure helper
  append_to_value(_env2 "${KEY}" "${VALUE}" "${_env}" ${MERGE_FN})

  # Write back to the caller's scope
  set(CTEST_ENVIRONMENT "${_env2}" PARENT_SCOPE)
endfunction()

#[=[
.. function:: test_env_append_value(TEST_NAME KEY VALUE MERGE_FN)

  Append/merge ``VALUE`` into the ``KEY`` entry of a test's CTest
  ``ENVIRONMENT`` property, preserving all other entries.

  Synopsis
    test_env_append_value(<test-name> <key> <value> <merge-fn>)

  Arguments
    - TEST_NAME : Name of an existing CTest test (e.g., ``echo_mytest``).
    - KEY       : Environment variable name to modify (e.g., ``FOO_FLAGS`` or
                  ``OPENSCAD_TEST_EXCLUDE_LINE``).
    - VALUE     : String to merge into the existing value for ``KEY``.
                  Quote it; for regex, prefer bracket quotes (e.g., ``[[...]]``).
    - MERGE_FN  : Name of a merge function with signature
                  ``(OUT, OLD, NEW)`` that writes the merged value to ``OUT``.

  Behaviour
    Reads the test's current ``ENVIRONMENT`` (a CMake list of ``KEY=VALUE``
    entries), extracts the last value for ``KEY`` if present, calls
    ``MERGE_FN`` to combine it with ``VALUE``, escapes semicolons in the
    result, and writes the updated list back to the test property.

  Notes
    - Call **after** the test is created (e.g., after ``add_test(...)`` or
      ``add_cmdline_test(...)``); otherwise CMake will error that the test
      does not exist.
    - ``MERGE_FN`` must be an existing CMake command/function; common choices:
      ``regex_alt`` (non-capturing alternation) or ``str_concat`` (plain
      concatenation).

  Example
    set(_extra_re [[^\s*\*\*\* Excluding \d+ frames \*\*\*\s*$]])
    test_env_append_value(
      echo_recursion-test-function3
      OPENSCAD_TEST_EXCLUDE_LINE
      "${_extra_re}"
      regex_alt
    )
]=]
function(test_env_append_value TEST_NAME KEY VALUE MERGE_FN)
  get_property(_env TEST ${TEST_NAME} PROPERTY ENVIRONMENT)

  append_to_value(_env2 "${KEY}" "${VALUE}" "${_env}" ${MERGE_FN})

  # Replace the test's ENVIRONMENT with the merged list
  set_property(TEST ${TEST_NAME} PROPERTY ENVIRONMENT "${_env2}")
endfunction()

#[=[
.. function:: add_line_exclusion_for_test(TEST_BASENAME TEST_NAME REGEX)

  Merge a per-test line-ignore pattern into the test's
  ``OPENSCAD_TEST_EXCLUDE_LINE`` environment entry.

  Synopsis
    add_line_exclusion_for_test(<kind> <basename> <regex>)

  Arguments
    - TEST_BASENAME : Test kind/prefix used in CTest names, e.g. ``echo``,
                      ``dump``, ``render``. The targeted test is
                      ``<kind>_<basename>``.
    - TEST_NAME     : Name of the test (e.g., ``recursion-test-function3``).
    - REGEX         : Python regular expression that will exclude **entire
                      lines** when matched.  Prefer bracket quotes like
                      ``[[...]]`` to make the regex easier to read.

  Behaviour
    Composes the test name as ``${TEST_BASENAME}_${TEST_NAME}`` and calls
    ``test_env_append_value`` to merge ``REGEX`` into that test's
    ``OPENSCAD_TEST_EXCLUDE_LINE`` using ``regex_alt`` (non-capturing
    alternation). Only the named test's ``ENVIRONMENT`` property is updated.

  Notes
    - Call **after** the tests have been created (e.g., after
      ``add_cmdline_test(TEST_BASENAME ...)``).
    - You may invoke this multiple times; patterns accumulate via alternation.
    - If your pattern might include ``;``, keep it bracket-quoted to avoid
      list splitting.

  Examples
    # Ignore "*** Excluding N frames ***" for a single echo_* test
    add_line_exclusion_for_test(echo recursion-test-function3
      [[^TRACE:\s*\*\*\* Excluding \d+ frames \*\*\*\s*$]])

    # Add an additional pattern for the same test
    add_line_exclusion_for_test(echo recursion-test-function3
      [[^TRACE:.*\((?:\d+)\s+frames\s+(?:omitted|skipped)\).*$]])

    # Apply to a dump_* test instead of echo_*
    add_line_exclusion_for_test(dump examples_children
      [[^# debug line$]])
]=]
function(add_line_exclusion_for_test TEST_BASENAME TEST_NAME REGEX)
  test_env_append_value(
    "${TEST_BASENAME}_${TEST_NAME}"
    OPENSCAD_TEST_EXCLUDE_LINE
    "${REGEX}"
    regex_alt
  )
endfunction()


#
# This functions adds cmd-line tests given files.
#
# Usage add_cmdline_test(testbasename [EXE <executable>] [ARGS <args to exe>]
#                        [SCRIPT <script>]
#                        [EXPECTEDDIR <shared dir>] SUFFIX <suffix> FILES <test files>
#                        [EXPERIMENTAL])
#
# EXPERIMENTAL: If set, tag all tests as experimental
#
function(add_cmdline_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "OPENSCAD;STDIO;EXPERIMENTAL" "EXE;SCRIPT;SUFFIX;KERNEL;EXPECTEDDIR" "FILES;ARGS" ${ARGN})

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

  # python script implies Python3_EXECUTABLE
  if (TESTCMD_SCRIPT MATCHES \\.[Pp][Yy]$)
    set(TESTCMD_EXE ${Python3_EXECUTABLE} -Xutf8=1)
  endif()
  if (TESTCMD_OPENSCAD)
    set(TESTCMD_EXE ${OPENSCAD_BINPATH})
  endif()

  if (TESTCMD_EXE)
    set(TESTNAME_OPTION -t ${TESTCMD_BASENAME})
  else()
    # If no executable was specified, assume it was built by us and resides here
    set(TESTCMD_EXE ${CCBD}/${TESTCMD_BASENAME})
  endif()

  # Add tests from args
  foreach (SCADFILE ${TESTCMD_FILES})
    get_filename_component(FILE_BASENAME ${SCADFILE} NAME_WE)
    string(REPLACE " " "_" FILE_BASENAME ${FILE_BASENAME}) # Test names cannot include spaces
    set(TEST_FULLNAME "${TESTCMD_BASENAME}_${FILE_BASENAME}")

    if (TESTCMD_EXPERIMENTAL)
      set(TEST_IS_EXPERIMENTAL true)
    endif()

    # add global experimental options here
    set(EXPERIMENTAL_OPTION "")

    # 2D tests should be viewed from the top, not an angle.
    set(CAMERA_OPTION "")
    is_2d(${SCADFILE} IS2D)
    if (IS2D)
      set(CAMERA_OPTION "--camera=0,0,100,0,0,0" "--viewall" "--autocenter" "--projection=ortho")
    endif()

    # Handle configurations
    get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
    if (NOT FOUNDCONFIGS)
      set_test_config(Default FILES ${TEST_FULLNAME})
    endif()
    set_test_config(All FILES ${TEST_FULLNAME})
    list(FIND FOUNDCONFIGS Bugs FOUND)
    if (FOUND EQUAL -1)
      set_test_config(Good FILES ${TEST_FULLNAME})
    endif()
    get_test_config(${TEST_FULLNAME} CONFVAL)

    set(FILENAME_OPTION -f ${FILE_BASENAME})

    # Apply lazy-union to *all* tests for comprehensive testing of this experimental feature.
    # Would need all passing before making lazy-union non-experimental, but that's probably a long way off.
    # Mostly just breaks issues that export non-manifold/intersecting geometry without explicit union.
    #set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=lazy-union")

    # Enable vertex-object-renderers-indexing by default for all test if experimental build
    # if (EXPERIMENTAL)
    #   set(EXPERIMENTAL_OPTION ${EXPERIMENTAL_OPTION} "--enable=vertex-object-renderers-indexing")
    # endif()

    string(JOIN " " DBG_COMMAND_STR
      "add_test(" ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL}
      COMMAND ${Python3_EXECUTABLE} -Xutf8=1
      ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXE}
      -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION}
      ${TESTCMD_EXE} ${TESTCMD_SCRIPT} ${SCADFILE} ${CAMERA_OPTION}
      ${EXPERIMENTAL_OPTION} ${MANIFOLD_OPTION} ${TESTCMD_ARGS} ")"
    )

    # only add test if it is not experimental or if it is and experimental option is enabled
    if (NOT TEST_IS_EXPERIMENTAL OR EXPERIMENTAL)
      # Use cmake option "--log-level DEBUG" during top level config to see this
      message(DEBUG "${DBG_COMMAND_STR}")      
      add_test(NAME ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL}
        COMMAND ${Python3_EXECUTABLE} -Xutf8=1
        ${TEST_CMDLINE_TOOL_PY} ${COMPARATOR} -c ${IMAGE_COMPARE_EXE}
        -s ${TESTCMD_SUFFIX} ${EXTRA_OPTIONS} ${TESTNAME_OPTION} ${FILENAME_OPTION}
        ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" ${CAMERA_OPTION}
        ${EXPERIMENTAL_OPTION} ${MANIFOLD_OPTION} ${TESTCMD_ARGS}
      )
      set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT ${CTEST_ENVIRONMENT})
    else()
      message(DEBUG "Experimental Test not added: ${DBG_COMMAND_STR}")
    endif()
  endforeach()
endfunction()

#
function(add_failing_test TESTCMD_BASENAME)
  cmake_parse_arguments(TESTCMD "" "RETVAL;EXE;SCRIPT;SUFFIX" "FILES;ARGS" ${ARGN})

  if ("${TESTCMD_SUFFIX}" STREQUAL "") # Suffix "off" counts as a false value, so check directly for empty string.
    message(FATAL_ERROR "add_failing_test() requires SUFFIX to be set" )
  endif()
  if (NOT TESTCMD_EXE)
    set(TESTCMD_EXE ${Python3_EXECUTABLE})
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
    # Handle configurations
    unset(FOUNDCONFIGS)
    get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
    if (NOT FOUNDCONFIGS)
      set_test_config(Default FILES ${TEST_FULLNAME})
    endif()
    set_test_config(All FILES ${TEST_FULLNAME})
    unset(FOUNDCONFIGS)
    get_test_config(${TEST_FULLNAME} FOUNDCONFIGS)
    set(CONFVAL ${FOUNDCONFIGS})

    # The python script cannot extract the testname when given extra parameters
    if (TESTCMD_ARGS)
      set(FILENAME_OPTION -f ${FILE_BASENAME})
    endif()

    add_test(NAME ${TEST_FULLNAME} CONFIGURATIONS ${CONFVAL} COMMAND ${TESTCMD_EXE} ${TESTCMD_SCRIPT} "${SCADFILE}" -s ${TESTCMD_SUFFIX} ${TESTCMD_ARGS})
    set_property(TEST ${TEST_FULLNAME} PROPERTY ENVIRONMENT "${CTEST_ENVIRONMENT}")
  endforeach()
endfunction()

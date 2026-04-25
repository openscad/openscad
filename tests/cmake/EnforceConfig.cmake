# For multi-config generators (MSVC, Xcode), -C specifies build configuration (Debug/Release)
# For single-config generators (Unix Makefiles, Ninja), -C can specify test configuration
# To avoid conflicts, we now use LABELS (-L flag) for test configuration selection.
#
# Recommended usage:
#   Single-config: ctest -L Default (or -L Examples, -L Heavy, etc.)
#   Multi-config:  ctest -C Release -L Default
#
# For backwards compatibility with single-config generators using -C,
# we still default to the legacy behavior, but print a deprecation notice.

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(IS_MULTI_CONFIG)
  # Multi-config generator: -C is for build config, don't default test config
  # Tests should be selected with -L flag instead
  if(NOT CTEST_CONFIGURATION_TYPE)
    message(STATUS "Multi-config generator detected. Use 'ctest -C <build-config> -L <test-group>' to run tests.")
    message(STATUS "Example: ctest -C Release -L Default")
    message(STATUS "Available test groups: Default, All, Good, Heavy, Examples, Bugs")
  endif()
else()
  # Single-config generator: maintain legacy behavior but encourage migration to -L
  if(NOT CTEST_CONFIGURATION_TYPE)
    message(STATUS "No test configuration specified. Enforcing 'Default' test configuration.")
    message(STATUS "RECOMMENDED: Use 'ctest -L <test-group>' instead of 'ctest -C <test-group>'")
    message(STATUS "  Example: ctest -L Default")
    message(STATUS "  Available: Default, All, Good, Heavy, Examples, Bugs")
    set(CTEST_CONFIGURATION_TYPE Default)
  elseif(CTEST_CONFIGURATION_TYPE MATCHES "^(Default|All|Good|Heavy|Examples|Bugs)$")
    message(STATUS "Using test configuration: ${CTEST_CONFIGURATION_TYPE}")
    message(STATUS "RECOMMENDED: Migrate to 'ctest -L ${CTEST_CONFIGURATION_TYPE}' for better compatibility")
  endif()
endif()

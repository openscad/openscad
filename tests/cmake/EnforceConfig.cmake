# Test groups are selected with ctest -L <group> (Default, Examples, Heavy, etc.).
#
# On multi-config generators (Visual Studio, Xcode), ctest -C specifies the *build*
# configuration (Release, Debug, ...), not the test group. Use:
#   ctest -C <Release|Debug|...> -L <group>
#
# On single-config generators (Makefiles, Ninja), -C is not needed for build type;
# use ctest -L <group> only.

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(IS_MULTI_CONFIG)
  if(NOT CTEST_CONFIGURATION_TYPE)
    message(STATUS "Multi-config generator: use 'ctest -C <build-config> -L <test-group>' (example: ctest -C Release -L Default).")
    message(STATUS "Test groups: Default, All, Good, Heavy, Examples, Bugs")
  endif()
else()
  if(NOT CTEST_CONFIGURATION_TYPE)
    message(STATUS "Run tests by group with ctest -L <test-group> (example: ctest -L Default).")
    message(STATUS "Test groups: Default, All, Good, Heavy, Examples, Bugs")
  endif()
endif()

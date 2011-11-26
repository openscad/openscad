message("Enforcing config")
if(NOT CTEST_CONFIGURATION_TYPE)
  set(CTEST_CONFIGURATION_TYPE Default)
endif()

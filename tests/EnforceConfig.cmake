if(NOT CTEST_CONFIGURATION_TYPE)
  message("Enforcing Default test configuration. Use ctest -C <config> to override")
  set(CTEST_CONFIGURATION_TYPE Default)
endif()

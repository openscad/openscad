get_directory_property(DEFINES COMPILE_DEFINITIONS)

message(" ")
message("====================================")
message("OpenSCAD Build Configuration Summary")
message("====================================")
message(" ")
message("CMAKE_BUILD_TYPE:  ${CMAKE_BUILD_TYPE}")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message("SUFFIX:            ${SUFFIX}")
if (OPENSCAD_LIBRARIES)
  message("OPENSCAD_LIBRARIES: $ENV{OPENSCAD_LIBRARIES}")
endif()
message(" ")
message("Experimental Features: ${EXPERIMENTAL}")
message("Snapshot build:        ${SNAPSHOT}")
message("Headless build:        ${HEADLESS}")
message("NULLGL build:          ${NULLGL}")
message("Profiling/coverage:    ${PROFILE}")
message("CSG node id-prefix:    ${IDPREFIX}")
message("Language Server:       ${LANGUAGESERVER}")
message(" ")
message("lib3mf:                ${LIB3MF_API}")
message(" ")
message("Input Drivers")
if("ENABLE_HIDAPI" IN_LIST DEFINES)
  message("- HID API Driver (enabled)")
else()
  message("- HID API Driver (disabled)")
endif()
if("ENABLE_SPNAV" IN_LIST DEFINES)
  message("- Space Navigator Library Driver (enabled)")
else()
  message("- Space Navigator Library Driver (disabled)")
endif()
if("ENABLE_JOYSTICK" IN_LIST DEFINES)
  message("- Joystick Driver (enabled)")
else()
  message("- Joystick Driver (disabled)")
endif()
if("ENABLE_DBUS" IN_LIST DEFINES)
  message("- DBus Remote Driver (enabled)")
else()
  message("- DBus Remote Driver (disabled)")
endif()
message(" ")
message("DEFINES = ${DEFINES}")
message(" ")

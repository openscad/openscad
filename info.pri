message("Configuration")
message("=============")
message("")

contains(CONFIG, debug) {
  message("Debug mode: enabled")
} else {
  message("Debug mode: disabled")
}

contains(DEFINES, ENABLE_EXPERIMENTAL) {
  message("Experimental features: enabled")
} else {
  message("Experimental features: disabled")
}

message("")
message("Input Drivers")
contains(DEFINES, ENABLE_HIDAPI) {
  message("- HID API Driver (enabled)")
} else {
  message("- HID API Driver (disabled)")
}
contains(DEFINES, ENABLE_SPNAV) {
  message("- Space Navigator Library Driver (enabled)")
} else {
  message("- Space Navigator Library Driver (disabled)")
}
contains(DEFINES, ENABLE_JOYSTICK) {
  message("- Joystick Driver (enabled)")
} else {
  message("- Joystick Driver (disabled)")
}
contains(DEFINES, ENABLE_DBUS) {
  message("- DBus Remote Driver (enabled)")
} else {
  message("- DBus Remote Driver (disabled)")
}

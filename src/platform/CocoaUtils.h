#pragma once

#include <string>

class CocoaUtils
{
public:
  static void endApplication();
  static void nslog(const std::string& str, void *userdata);
  // Force the macOS appearance for the whole app.  Pass dark=true/false to
  // lock it; call resetAppearance() to restore system-managed mode so that
  // colorSchemeChanged fires on OS-level changes.
  static void setAppearance(bool dark);
  static void resetAppearance();
};

#pragma once

#include <string>

class CocoaUtils
{
public:
  static void endApplication();
  static void prepareOpenFileHandler();
  static void installOpenFileHandler();
  static void nslog(const std::string& str, void *userdata);
};

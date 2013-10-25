#ifndef COCOAUTILS_H_
#define COCOAUTILS_H_

#include <string>

class CocoaUtils
{
public:
	static void endApplication();
  static void nslog(const std::string &str, void *userdata);
};

#endif

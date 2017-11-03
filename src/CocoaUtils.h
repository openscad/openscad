#pragma once

#include <string>

class CocoaUtils
{
public:
	static void endApplication();
	static void nslog(const std::string &str, void *userdata);
};

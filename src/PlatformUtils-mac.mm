#include "PlatformUtils.h"
#import <Foundation/Foundation.h>

std::string PlatformUtils::pathSeparatorChar()
{
	return ":";
}

std::string PlatformUtils::documentsPath()
{
  return std::string([[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] UTF8String]);
}

std::string PlatformUtils::userConfigPath()
{
    return "";
}

void PlatformUtils::ensureStdIO(void) {}


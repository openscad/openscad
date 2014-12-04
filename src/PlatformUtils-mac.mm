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
	NSError *error;
	NSURL *appSupportDir = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:&error];
	return std::string([[appSupportDir path] UTF8String]) + std::string("/") + PlatformUtils::OPENSCAD_FOLDER_NAME;
}

unsigned long PlatformUtils::stackLimit()
{
  struct rlimit limit;        
  
  int ret = getrlimit(RLIMIT_STACK, &limit);
  if (ret == 0) {
    if (limit.rlim_cur > STACK_BUFFER_SIZE) {
      return limit.rlim_cur - STACK_BUFFER_SIZE;
    }
    if (limit.rlim_max > STACK_BUFFER_SIZE) {
      return limit.rlim_max - STACK_BUFFER_SIZE;
    }
  }
  
  return STACK_LIMIT_DEFAULT;
}

void PlatformUtils::ensureStdIO(void) {}


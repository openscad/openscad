#include "PlatformUtils.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <boost/lexical_cast.hpp>

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

std::string PlatformUtils::sysinfo()
{
  std::string result;
  
  result += "Mac OS X ";
  result += [[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String];
  
  int64_t physical_memory;
  int32_t numcpu;
  size_t length64 = sizeof(int64_t);
  size_t length32 = sizeof(int32_t);;
  
  sysctlbyname("hw.memsize", &physical_memory, &length64, NULL, 0);
  sysctlbyname("hw.physicalcpu", &numcpu, &length32, NULL, 0);
  
  result += " ";
  result += boost::lexical_cast<std::string>(numcpu);
  result += " CPU";
  if (numcpu > 1) result += "s";
  
  result += " ";
  result += PlatformUtils::toMemorySizeString(physical_memory, 2);
  result += " RAM";
  
  return result;
}

void PlatformUtils::ensureStdIO(void) {}


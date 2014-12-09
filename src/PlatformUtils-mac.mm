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

std::string PlatformUtils::sysinfo()
{
    std::string result;

#if 0    
    struct utsname osinfo;
    if (uname(&osinfo) == 0) {
	result += osinfo.sysname;
	result += " ";
	result += osinfo.release;
	result += " ";
	result += osinfo.version;
	result += " ";
	result += osinfo.machine;
    } else {
#endif
        result += "Unknown MacOS";
#if 0
    }
    
    long numcpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (numcpu > 0) {
	result += " ";
	result += boost::lexical_cast<std::string>(numcpu);
	result += " CPU";
	if (numcpu > 1) {
	    result += "s";
	}
    }
    
    long pages = sysconf(_SC_PHYS_PAGES);
    long pagesize = sysconf(_SC_PAGE_SIZE);
    if ((pages > 0) && (pagesize > 0)) {
	result += " ";
	result += PlatformUtils::toMemorySizeString(pages * pagesize, 2);
	result += " RAM";
    }
#endif

    return result;
}

void PlatformUtils::ensureStdIO(void) {}


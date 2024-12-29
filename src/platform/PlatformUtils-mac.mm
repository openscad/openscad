#include "PlatformUtils.h"

#include <sstream>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <boost/lexical_cast.hpp>

#import <Foundation/Foundation.h>

#include "version.h"

std::string PlatformUtils::pathSeparatorChar()
{
  return ":";
}

std::string PlatformUtils::userDocumentsPath()
{
  return documentsPath();
}

std::string PlatformUtils::documentsPath()
{
  return std::string([[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] UTF8String]);
}

std::string PlatformUtils::userConfigPath()
{
  NSError *error = nullptr;
  NSURL *appSupportDir = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:&error];
  if (error) {
    return "";
  }
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

const std::string PlatformUtils::user_agent()
{
  std::ostringstream result;

  result << "OpenSCAD/" << openscad_detailedversionnumber
         << " (" << sysinfo(false) << ")";

  return result.str();
}

const std::string PlatformUtils::sysinfo(bool extended)
{
  std::ostringstream result;
  
  NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
  struct utsname name;
  uname(&name);
  result << "macOS " << version.majorVersion << "." << version.minorVersion << "." << version.patchVersion
	 << " " << name.machine;

  size_t modellen = 0;
  sysctlbyname("hw.model", nullptr, &modellen, nullptr, 0);
  std::string model;
  if (modellen > 0) {
    model.resize(modellen - 1);
    sysctlbyname("hw.model", model.data(), &modellen, nullptr, 0);
    result << " " << model;
  }

  if (extended) {
    int32_t numcpu;
    size_t length32 = sizeof(int32_t);;
    sysctlbyname("hw.physicalcpu", &numcpu, &length32, nullptr, 0);
    int64_t physical_memory;
    size_t length64 = sizeof(int64_t);
    sysctlbyname("hw.memsize", &physical_memory, &length64, nullptr, 0);
  
    result << " " << numcpu << " CPU";
    if (numcpu > 1) result << "s"; 
    result << " " << PlatformUtils::toMemorySizeString(physical_memory, 2) << " RAM ";
  }

  return result.str();
}

void PlatformUtils::ensureStdIO(void) {}

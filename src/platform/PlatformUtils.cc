#include "platform/PlatformUtils.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/printutils.h"

#ifdef OPENSCAD_SUFFIX
#define RESOURCE_FOLDER(path) path OPENSCAD_SUFFIX
#else
#define RESOURCE_FOLDER(path) path
#endif

namespace {
bool path_initialized = false;
fs::path applicationpath;
fs::path resourcespath;
}  // namespace

const char *PlatformUtils::OPENSCAD_FOLDER_NAME = "OpenSCAD";

static fs::path lookupResourcesPath()
{
  // resourcedir defaults to applicationPath
  fs::path resourcedir(applicationpath);
  PRINTDB("Looking up resource folder with application path %s", quotePath(resourcedir));

#ifdef __APPLE__
  const char *searchpath[] = {"../Resources",                        // Resources can be bundled on Mac.
                              "../../..",                            // Dev location
                              "../../../..",                         // Test location (cmake)
                              "..",                                  // Test location
                              RESOURCE_FOLDER("../share/openscad"),  // Unix mode
                              nullptr};
#else
#ifdef _WIN32
  const char *searchpath[] = {".",                                   // Release location
                              RESOURCE_FOLDER("../share/openscad"),  // MSYS2 location
                              "..",                                  // Dev location
                              nullptr};
#else
  const char *searchpath[] = {RESOURCE_FOLDER("../share/openscad"),
                              RESOURCE_FOLDER("../../share/openscad"),
                              ".",
                              "..",
                              "../..",
                              nullptr};
#endif  // ifdef _WIN32
#endif  // ifdef __APPLE__

  fs::path tmpdir;
  for (int a = 0; searchpath[a] != nullptr; ++a) {
    tmpdir = resourcedir / searchpath[a];

    // The resource folder is the folder which contains "color-schemes" (as well as
    // "examples" and "locale", and optionally "libraries" and "fonts")
    const fs::path checkdir = tmpdir / "color-schemes";
    PRINTDB("Checking %s", quotePath(checkdir));

    if (is_directory(checkdir)) {
      resourcedir = tmpdir;
      PRINTDB("Found resource folder %s", quotePath(tmpdir));
      break;
    }
  }

#ifndef __EMSCRIPTEN__
  resourcedir = fs::canonical(resourcedir);
#endif
  PRINTDB("Using resource folder %s", quotePath(resourcedir));
  return resourcedir;
}

void PlatformUtils::registerApplicationPath(const fs::path& apppath)
{
  applicationpath = apppath;
  resourcespath = lookupResourcesPath();
  path_initialized = true;
}

fs::path PlatformUtils::applicationPath()
{
  if (!path_initialized) {
    throw std::runtime_error("PlatformUtils::applicationPath(): application path not initialized!");
  }
  return applicationpath;
}

bool PlatformUtils::createUserLibraryPath()
{
  fs::path path = PlatformUtils::userLibraryPath();
  bool OK = false;
  try {
    if (!fs::exists(path)) {
      LOG("Creating library folder %1$s", quotePath(path));
      OK = fs::create_directories(path);
    }
    if (!OK) {
      LOG(message_group::Error, "Cannot create %1$s", quotePath(path));
    }
  } catch (const fs::filesystem_error& ex) {
    LOG(message_group::Error, "%1$s", ex.what());
  }
  return OK;
}

fs::path PlatformUtils::userPath(const std::string& name)
{
  fs::path path;
  try {
    path = PlatformUtils::documentsPath();
    if (!fs::exists(path)) return {};
#ifndef __EMSCRIPTEN__
    path = fs::canonical(path);
#endif
    // LOG(message_group::NONE,,"path size %1$i",fs::stringy(path).size());
    // LOG(message_group::NONE,,"lib path found: [%1$s]",path);
    if (path.empty()) return {};
    path /= OPENSCAD_FOLDER_NAME;
    path /= name;
    // LOG(message_group::NONE,,"Appended path %1$s",path);
    // LOG(message_group::NONE,,"Exists: %1$i",fs::exists(path));
  } catch (const fs::filesystem_error& ex) {
    LOG(message_group::Error, "%1$s", ex.what());
  }
  return path;
}

fs::path PlatformUtils::userLibraryPath()
{
  return userPath("libraries");
}

fs::path PlatformUtils::userExamplesPath()
{
  return userPath("examples");
}

fs::path PlatformUtils::backupPath()
{
  fs::path path;
  try {
    path = PlatformUtils::documentsPath();
    if (!fs::exists(path)) return {};
#ifndef __EMSCRIPTEN__
    path = fs::canonical(path);
#endif
    if (path.empty()) return {};
    path /= OPENSCAD_FOLDER_NAME;
    path /= "backups";
  } catch (const fs::filesystem_error& ex) {
    LOG(message_group::Error, "%1$s", ex.what());
  }
  return path;
}

bool PlatformUtils::createBackupPath()
{
  fs::path path = PlatformUtils::backupPath();
  bool OK = false;
  try {
    if (!fs::exists(path)) {
      OK = fs::create_directories(path);
    }
    if (!OK) {
      LOG(message_group::Error, "Cannot create %1$s", quotePath(path));
    }
  } catch (const fs::filesystem_error& ex) {
    LOG(message_group::Error, "%1$s", ex.what());
  }
  return OK;
}

// This is the built-in read-only resources path
fs::path PlatformUtils::resourceBasePath()
{
  if (!path_initialized) {
    throw std::runtime_error("PlatformUtils::resourcesPath(): application path not initialized!");
  }
  return resourcespath;
}

fs::path PlatformUtils::resourcePath(const std::string& resource)
{
  fs::path base(resourceBasePath());
  if (!fs::is_directory(base)) {
    return {};
  }

  fs::path resource_dir = base / resource;
  if (!fs::is_directory(resource_dir)) {
    return {};
  }

  return resource_dir;
}

int PlatformUtils::setenv(const char *name, const std::string svalue, int overwrite)
{
  const char *cvalue = svalue.c_str();
#if defined(_WIN32)
  const char *ptr = getenv(cvalue);
  if ((overwrite == 0) && (ptr != nullptr)) {
    return 0;
  }

  char buf[4096];
  snprintf(buf, sizeof(buf), "%s=%s", name, cvalue);
  return _putenv(buf);
#else
  return ::setenv(name, cvalue, overwrite);
#endif
}

std::string PlatformUtils::toMemorySizeString(uint64_t bytes, int digits)
{
  static const char *units[] = {"B", "kB", "MB", "GB", "TB", nullptr};

  int idx = 0;
  double val = bytes;
  while (true) {
    if (val < 1024.0) {
      break;
    }
    if (units[idx + 1] == nullptr) {
      break;
    }
    idx++;
    val /= 1024.0;
  }

  boost::format fmt("%f %s");
  fmt % boost::io::group(std::setprecision(digits), val) % units[idx];
  return fmt.str();
}

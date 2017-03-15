#include <string>
#include <fstream>
#include <streambuf>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/utsname.h>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "PlatformUtils.h"

namespace fs=boost::filesystem;

std::string PlatformUtils::pathSeparatorChar()
{
	return ":";
}

std::string PlatformUtils::documentsPath()
{
	const char *home = getenv("HOME");
	if (home) {
		fs::path docpath(home);
		docpath = docpath / ".local" / "share";
		return docpath.generic_string();
	}
	else {
		return "";
	}
}

// see http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
std::string PlatformUtils::userConfigPath()
{
    fs::path config_path;
    
    const char *xdg_env = getenv("XDG_CONFIG_HOME");
    if (xdg_env && fs::exists(fs::path(xdg_env))) {
	config_path = fs::path(xdg_env) / OPENSCAD_FOLDER_NAME;
    } else {
	const char *home = getenv("HOME");
	if (home) {
	    config_path = fs::path(home) / ".config" / OPENSCAD_FOLDER_NAME;
	}
    }

    if (fs::is_directory(config_path)) {
	return fs::absolute(config_path).generic_string();
    }
    
    return "";
}

unsigned long PlatformUtils::stackLimit()
{
    struct rlimit limit;

    int ret = getrlimit(RLIMIT_STACK, &limit);
    if (ret == 0) {
        if (limit.rlim_cur == RLIM_INFINITY) {
	  return STACK_LIMIT_DEFAULT;
        }
	if (limit.rlim_cur > STACK_BUFFER_SIZE) {
	    return limit.rlim_cur - STACK_BUFFER_SIZE;
	}
        if (limit.rlim_max == RLIM_INFINITY) {
          return STACK_LIMIT_DEFAULT;
        }
	if (limit.rlim_max > STACK_BUFFER_SIZE) {
	    return limit.rlim_max - STACK_BUFFER_SIZE;
	}
    }

    return STACK_LIMIT_DEFAULT;
}

static std::string readText(const std::string &path)
{
    std::ifstream s(path.c_str());
    s.seekg(0, std::ios::end);
    if (s.fail() || s.tellg() > 4096) {
	return "";
    }
    s.seekg(0, std::ios::beg);

    std::string text((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
    return text;
}

/**
 * Check /etc/os-release as defined by systemd.
 * @see http://0pointer.de/blog/projects/os-release.html
 * @see http://www.freedesktop.org/software/systemd/man/os-release.html
 * @return the PRETTY_NAME from the os-release file or an empty string.
 */
static std::string checkOsRelease()
{
    std::string os_release(readText("/etc/os-release"));

    boost::smatch results;
    boost::regex pretty_name("^PRETTY_NAME=\"([^\"]+)\"");
    if (boost::regex_search(os_release, results, pretty_name)) {
	return results[1];
    }

    return "";
}

static std::string checkEtcIssue()
{
    std::string issue(readText("/etc/issue"));

    boost::regex nl("\n.*$");
    issue = boost::regex_replace(issue, nl, "");
    boost::regex esc("\\\\.");
    issue = boost::regex_replace(issue, esc, "");
    boost::algorithm::trim(issue);
    
    return issue;
}

static std::string detectDistribution()
{
    std::string osrelease = checkOsRelease();
    if (!osrelease.empty()) {
	return osrelease;
    }

    std::string etcissue = checkEtcIssue();
    if (!etcissue.empty()) {
	return etcissue;
    }
    
    return "";
}

std::string PlatformUtils::sysinfo(bool extended)
{
    std::string result;
    
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
			result += "Unknown Linux";
    }
    
    std::string distribution = detectDistribution();
    if (!distribution.empty()) {
			result += " ";
			result += distribution;
    }

		if (extended) {
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
		}
		
    return result;
}

void PlatformUtils::ensureStdIO(void) {}


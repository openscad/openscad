#include <sys/resource.h>

#include "PlatformUtils.h"
#include "boosty.h"

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
		return boosty::stringy(docpath);
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
	return boosty::stringy(boosty::absolute(config_path));
    }
    
    return "";
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


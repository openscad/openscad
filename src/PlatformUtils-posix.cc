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
	config_path = fs::path(xdg_env) / "OpenSCAD";
    } else {
	const char *home = getenv("HOME");
	if (home) {
	    config_path = fs::path(home) / ".config" / "OpenSCAD";
	}
    }

    if (fs::is_directory(config_path)) {
	return boosty::stringy(boosty::absolute(config_path));
    }
    
    return "";
}

void PlatformUtils::ensureStdIO(void) {}


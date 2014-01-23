#include "PlatformUtils.h"
#include "boosty.h"

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

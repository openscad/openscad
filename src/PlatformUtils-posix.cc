#include "PlatformUtils.h"
#include "boosty.h"

std::string PlatformUtils::documentsPath()
{
	fs::path docpath(getenv("HOME"));
	docpath = docpath / ".local" / "share";

	return boosty::stringy(docpath);
}

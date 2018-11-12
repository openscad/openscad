#pragma once

#include <sstream>

namespace OpenSCAD {

struct library_version_number
{
	const long major;
	const long minor;
	const long micro;
};

const auto get_version_string = [](const library_version_number& header_version, const library_version_number& runtime_version)
{
	std::ostringstream version_stream;

	version_stream << header_version.major << '.' << header_version.minor << '.' << header_version.micro;
	const bool match = (header_version.major == runtime_version.major && header_version.minor == runtime_version.minor && header_version.micro == runtime_version.micro);
	if (!match) {
		version_stream << " (runtime: " << runtime_version.major << '.' << runtime_version.minor << '.' << runtime_version.micro << ')';
	}
	const std::string version = version_stream.str();
	return version;
};

}
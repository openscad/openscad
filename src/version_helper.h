#pragma once

#include <sstream>

namespace OpenSCAD {

struct library_version_number
{
	const unsigned int major;
	const unsigned int minor;
	const unsigned int micro;
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

const auto get_version = [](const std::string& header_version, const std::string& runtime_version)
{
	std::ostringstream version_stream;

	version_stream << header_version;
	if (header_version != runtime_version) {
		version_stream << " (runtime: " << runtime_version << ')';
	}
	const std::string version = version_stream.str();
	return version;
};

}
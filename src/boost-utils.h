#pragma once

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

// FIXME: boostfs_relative_path() has been replaced by
// boostfs_uncomplete(), but kept around for now.
fs::path boostfs_relative_path(const fs::path &path, const fs::path &relative_to);
fs::path boostfs_normalize(const fs::path &path);
fs::path boostfs_uncomplete(fs::path const p, fs::path const base);

#include <boost/cast.hpp>
#include <sstream>

/* Convert number types but print WARNING for failures during
   conversion. This is useful for situations where it is important to not
   fail silently during casting or conversion. (For example, accidentally
   converting 64 bit types to 32 bit types, float to int, etc).
   For positive overflow, return max of Tout template type
   For negative overflow, return min of Tout template type
   On other conversion failures, return 0. */
template <class Tout, class Tin> Tout boost_numeric_cast(Tin input)
{
	Tout result = 0;
	std::stringstream status;
	status.str("ok");
	try {
		result = boost::numeric_cast<Tout>(input);
	}
	catch (boost::numeric::negative_overflow &e) {
		status << e.what();
		result = std::numeric_limits<Tout>::min();
	}
	catch (boost::numeric::positive_overflow &e) {
		status << e.what();
		result = std::numeric_limits<Tout>::max();
	}
	catch (boost::numeric::bad_numeric_cast &e) {
		status << e.what();
		result = 0;
	}
	if (status.str() != "ok") {
		std::stringstream tmp;
		tmp << input;
		PRINTB("WARNING: problem converting this number: %s", tmp.str());
		PRINTB("WARNING: %s", status.str());
		PRINTB("WARNING: setting result to %u", result);
	}
	return result;
}



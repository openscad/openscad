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

// convert number types (for example double to int) but print WARNING
// for failures during conversion. >0 Overflow: return max of Tout,
// <0 overflow return min, otherwise return 0. use std::numeric_limits
template <class Tout,class Tin> Tout boost_numeric_cast( Tin input )
{
	Tout result = 0;
	std::stringstream status;
	status.str("ok");
	try {
		result = boost::numeric_cast<Tout>(input);
	} catch (boost::numeric::negative_overflow& e) {
		status << e.what();
		result = std::numeric_limits<Tout>::min();
	} catch (boost::numeric::positive_overflow& e) {
		status << e.what();
		result = std::numeric_limits<Tout>::max();
	} catch (boost::numeric::bad_numeric_cast& e) {
		status << e.what();
		result = 0;
	}
	if (status.str()!="ok") {
		std::stringstream tmp;
		tmp << input;
		PRINTB("WARNING: problem converting this number: %s",tmp.str());
		PRINTB("WARNING: %s", status.str() );
		PRINTB("WARNING: setting result to %u",result);
	}
	return result;
}



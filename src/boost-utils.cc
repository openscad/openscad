#include "boosty.h"
#include "boost-utils.h"
#include <stdio.h>
#include <iostream>

namespace fs=boost::filesystem;
// If the given (absolute) path is relative to the relative_to path, return a new
// relative path. Will normalize the given path first
fs::path boostfs_relative_path(const fs::path &path, const fs::path &relative_to)
{
	// create absolute paths
	fs::path p = fs::absolute(boostfs_normalize(path));
	fs::path r = fs::absolute(relative_to);
	
	// if root paths are different, return absolute path
	if (p.root_path() != r.root_path())
		return p;
	
	// initialize relative path
	fs::path result;
	
	// find out where the two paths diverge
	fs::path::const_iterator itr_path = p.begin();
	fs::path::const_iterator itr_relative_to = r.begin();
	while (*itr_path == *itr_relative_to && itr_path != p.end() && itr_relative_to != r.end()) {
		++itr_path;
		++itr_relative_to;
	}
	
	// add "../" for each remaining token in relative_to
	if (itr_relative_to != r.end()) {
		++itr_relative_to;
		while (itr_relative_to != r.end()) {
			result /= "..";
			++itr_relative_to;
		}
	}
	
	// add remaining path
	while (itr_path != p.end()) {
		result /= *itr_path;
		++itr_path;
	}
	
	return result;
}

// Will normalize the given path, i.e. remove any redundant ".." path elements.
fs::path boostfs_normalize(const fs::path &path)
{
	fs::path absPath = fs::absolute(path);
	fs::path::iterator it = absPath.begin();
	fs::path result = *it;
	if (it!=absPath.end()) it++;

	// Get canonical version of the existing part
	for(;exists(result) && it != absPath.end(); ++it) {
		result /= *it;
	}
	result = boosty::canonical(result.parent_path());
	if (it!=absPath.begin()) it--;

	// For the rest remove ".." and "." in a path with no symlinks
	for (; it != absPath.end(); ++it) {
		// Just move back on ../
		if (*it == "..") {
			result = result.parent_path();
		}
		// Ignore "."
		else if (*it != ".") {
			// Just cat other path entries
			result /= *it;
		}
	}

	return result;
}

/**
 * https://svn.boost.org/trac/boost/ticket/1976#comment:2
 * 
 * "The idea: uncomplete(/foo/new, /foo/bar) => ../new
 *  The use case for this is any time you get a full path (from an open dialog, perhaps)
 *  and want to store a relative path so that the group of files can be moved to a different
 *  directory without breaking the paths. An IDE would be a simple example, so that the
 *  project file could be safely checked out of subversion."
 * 
 * ALGORITHM:
 *  iterate path and base
 * compare all elements so far of path and base
 * whilst they are the same, no write to output
 * when they change, or one runs out:
 *   write to output, ../ times the number of remaining elements in base
 *   write to output, the remaining elements in path
 */
fs::path
boostfs_uncomplete(fs::path const p, fs::path const base)
{
	if (p == base) return "./";
	/*!! this breaks stuff if path is a filename rather than a directory,
		which it most likely is... but then base shouldn't be a filename so... */

	// create absolute paths
	fs::path abs_p = fs::absolute(boostfs_normalize(p));
	fs::path abs_base = fs::absolute(base);

	fs::path from_path, from_base, output;

	fs::path::iterator path_it = abs_p.begin(),    path_end = abs_p.end();
	fs::path::iterator base_it = abs_base.begin(), base_end = abs_base.end();

	// check for emptiness
	if ((path_it == path_end) || (base_it == base_end)) {
		throw std::runtime_error("path or base was empty; couldn't generate relative path");
	}

#ifdef WIN32
	// drive letters are different; don't generate a relative path
	if (*path_it != *base_it) return p;

	// now advance past drive letters; relative paths should only go up
	// to the root of the drive and not past it
	++path_it, ++base_it;
#endif

	// Cache system-dependent dot, double-dot and slash strings
	const std::string _dot  = ".";
	const std::string _dots = "..";
	const std::string _sep = "/";

	// iterate over path and base
	while (true) {

		// compare all elements so far of path and base to find greatest common root;
		// when elements of path and base differ, or run out:
		if ((path_it == path_end) || (base_it == base_end) || (*path_it != *base_it)) {

			// write to output, ../ times the number of remaining elements in base;
			// this is how far we've had to come down the tree from base to get to the common root
			for (; base_it != base_end; ++base_it) {
				if (*base_it == _dot)
					continue;
				else if (*base_it == _sep)
					continue;

				output /= "../";
			}

			// write to output, the remaining elements in path;
			// this is the path relative from the common root
			fs::path::iterator path_it_start = path_it;
			for (; path_it != path_end; ++path_it) {
				if (path_it != path_it_start) output /= "/";
				if (*path_it == _dot) continue;
				if (*path_it == _sep) continue;
				output /= *path_it;
			}
			break;
		}

		// add directory level to both paths and continue iteration
		from_path /= fs::path(*path_it);
		from_base /= fs::path(*base_it);

		++path_it, ++base_it;
	}

	return output;
}

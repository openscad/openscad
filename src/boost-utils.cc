#include "boost-utils.h"
#include <stdio.h>
#include <iostream>

fs::path relativePath(const fs::path &path, const fs::path &relative_to)
{
	// create absolute paths
	fs::path p = fs::absolute(path);
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

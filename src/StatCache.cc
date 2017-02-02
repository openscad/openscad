/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "StatCache.h"
#include "printutils.h"

#include <sys/stat.h>
#include <string>
#include <unordered_map>
#include <ctime>

const float stale = 0.190;  // Maximum lifetime of a cache entry chosen to be shorter than the automatic reload poll time

struct CacheEntry {
	struct stat st;         // result from stat
	clock_t timestamp;      // the time stat was called
};

typedef std::unordered_map<std::string, CacheEntry> StatMap;

static StatMap statMap;

int StatCache::stat(const char *path, struct stat *st)
{
	StatMap::iterator iter = statMap.find(path);
	if (iter != statMap.end()) {                      // Have we got an entry for this file?
		clock_t age = clock() - iter->second.timestamp; // How old is it?
		if(float(age) / CLOCKS_PER_SEC < stale) {       // Not stale yet so return it
			*st = iter->second.st;
			return 0;
		}
		statMap.erase(iter);                            // Remove state entry
	}
	CacheEntry entry;                                 // Make a new entry
	if (int rv = ::stat(path, &entry.st)) return rv;  // stat failed
	entry.timestamp = clock();                        // Add the current time
	statMap[path] = entry;
	*st = entry.st;
	return 0;
}   


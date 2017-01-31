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

typedef std::unordered_map<std::string, struct stat> StatMap;

static StatMap statMap;
static int hits, misses;
static bool printed = false;

void StatCache::clear(void) {
    if(misses && !printed) {
        PRINTDB("StatCache: %d hits, %d misses", hits % misses);
        printed = true;
    }
    statMap.clear();
    hits = misses = 0;
}
    
    
int StatCache::stat(const char *path, struct stat *st) {
    StatMap::iterator iter = statMap.find(path);
    if(iter != statMap.end()) {
        *st = iter->second;
        ++hits;
        return 0;
    }
    struct stat buff;
    if(int rv = ::stat(path, &buff))
        return rv;
    statMap.insert(std::make_pair(std::string(path), buff));
    *st = buff;
    ++misses;
    return 0;
}   


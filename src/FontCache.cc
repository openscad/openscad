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

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "boosty.h"
#include "FontCache.h"

namespace fs = boost::filesystem;

FontCache * FontCache::self = NULL;

FontCache::FontCache()
{
	init_ok = false;

	config = FcInitLoadConfigAndFonts();
	if(!config) {
		PRINT("Can't initialize fontconfig library, text() objects will not be rendered");
		return;
	}

	add_font_dir("/System/Library/Fonts");
	const char *home = getenv("HOME");
	if (home) {
		add_font_dir(std::string(home) + "/Library/Fonts");
		add_font_dir(std::string(home) + "/.fonts");
	}
	const char *windir = getenv("WinDir");
	if (windir) {
		add_font_dir(std::string(windir) + "\\Fonts");
	}
	
	const FT_Error error = FT_Init_FreeType(&library);
	if (error) {
		PRINT("Can't initialize freetype library, text() objects will not be rendered");
		return;
	}
	
	init_ok = true;
}

FontCache::~FontCache()
{
}

FontCache * FontCache::instance()
{
	if (!self) {
		self = new FontCache();
	}
	return self;
}

void FontCache::register_font_file(std::string path) {
	if (!FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8 *>(path.c_str()))) {
		PRINTB("Can't register font '%s'", path);
	}
}

void FontCache::add_font_dir(std::string path) {
	if (!fs::is_directory(path)) {
		return;
	}
	if (!FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8 *>(path.c_str()))) {
		PRINTB("Can't register font directory '%s'", path);
	}
}

bool FontCache::is_init_ok()
{
	return init_ok;
}

void FontCache::clear()
{
	cache.clear();
}

void FontCache::dump_cache(std::string info)
{
	std::cout << info << ":";
	for (cache_t::iterator it = cache.begin();it != cache.end();it++) {
		std::cout << " " << (*it).first << " (" << (*it).second.second << ")";
	}
	std::cout << std::endl;
}

void FontCache::check_cleanup()
{
	if (cache.size() < MAX_NR_OF_CACHE_ENTRIES) {
		return;
	}

	cache_t::iterator pos = cache.begin()++;
	for (cache_t::iterator it = cache.begin();it != cache.end();it++) {
		if ((*pos).second.second > (*it).second.second) {
			pos = it;
		}
	}
	FT_Done_Face((*pos).second.first);
	cache.erase(pos);
}

FT_Face FontCache::get_font(std::string font)
{
	FT_Face face;
	cache_t::iterator it = cache.find(font);
	if (it == cache.end()) {
		face = find_face(font);
		if (!face) {
			return NULL;
		}
		check_cleanup();
	} else {
		face = (*it).second.first;
	}
	cache[font] = cache_entry_t(face, time(NULL));
	return face;
}

FT_Face FontCache::find_face(std::string font)
{
	FT_Face face;
	
	face = find_face_fontconfig(font);
	return face ? face : find_face_in_path_list(font);
}	
	
FT_Face FontCache::find_face_in_path_list(std::string font)
{
	const char *env_font_path = getenv("OPENSCAD_FONT_PATH");
	
	std::string paths = (env_font_path == NULL) ? "/usr/share/fonts/truetype" : env_font_path;
	typedef boost::split_iterator<std::string::iterator> string_split_iterator;
	for (string_split_iterator it = boost::make_split_iterator(paths, boost::first_finder(":", boost::is_iequal()));it != string_split_iterator();it++) {
		fs::path p(boost::copy_range<std::string>(*it));
		if (fs::exists(p)) {
			std::string path = boosty::absolute(p).string();
			FT_Face face = find_face_in_path(path, font);
			if (face) {
				return face;
			}
		}
	}
	return NULL;
}

FT_Face FontCache::find_face_fontconfig(std::string font)
{
	FcResult result;
	
	FcPattern *pattern = FcNameParse((unsigned char *)font.c_str());
	
	FcValue true_value;
	true_value.type = FcTypeBool;
	true_value.u.b = true;
	
	FcPatternAdd(pattern, FC_OUTLINE, true_value, true);
	FcPatternAdd(pattern, FC_SCALABLE, true_value, true);
	FcDefaultSubstitute(pattern);
	FcConfigSubstitute(config, pattern, FcMatchFont);
	FcPattern *match = FcFontMatch(config, pattern, &result);
	
	FcValue file_value;
	FcPatternGet(match, FC_FILE, 0, &file_value);

	FT_Face face;
	FT_Error error = FT_New_Face(library, (const char *)file_value.u.s, 0, &face);

	FcPatternDestroy(pattern);
	FcPatternDestroy(match);

	return error ? NULL : face;
}

FT_Face FontCache::find_face_in_path(std::string path, std::string font)
{
	FT_Error error;
	
	if (!fs::is_directory(path)) {
		PRINTB("Font path '%s' does not exist or is not a directory.", path);
	}

	for (fs::recursive_directory_iterator it(path);it != fs::recursive_directory_iterator();it++) {
		fs::directory_entry entry = (*it);
		if (fs::is_regular(entry.path())) {
			FT_Face face;
			error = FT_New_Face(library, entry.path().c_str(), 0, &face);
			if (error) {
				continue;
			}
			const char *name = FT_Get_Postscript_Name(face);
			if (font == name) {
				return face;
			}
			FT_Done_Face(face);
		}
	}
	return NULL;
}

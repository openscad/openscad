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

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "FontCache.h"
#include "PlatformUtils.h"
#include "parsersettings.h"

extern std::vector<std::string> librarypath;

std::vector<std::string> fontpath;

namespace fs = boost::filesystem;

static bool FontInfoSortPredicate(const FontInfo& fi1, const FontInfo& fi2)
{
	return (fi1 < fi2);
}

FontInfo::FontInfo(std::string family, std::string style, std::string file) : family(family), style(style), file(file)
{
}

FontInfo::~FontInfo()
{
}

bool FontInfo::operator<(const FontInfo &rhs) const
{
	if (family < rhs.family) {
		return true;
	}
	if (style < rhs.style) {
		return true;
	}
	return file < rhs.file;
}

std::string FontInfo::get_family() const
{
	return family;
}

std::string FontInfo::get_style() const
{
	return style;
}

std::string FontInfo::get_file() const
{
	return file;
}

FontCache * FontCache::self = NULL;
const std::string FontCache::DEFAULT_FONT("Liberation Sans:style=Regular");

FontCache::FontCache()
{
	init_ok = false;

	// If we've got a bundled fonts.conf, initialize fontconfig with our own config
	// by overriding the built-in fontconfig path.
	// For system installs and dev environments, we leave this alone
	fs::path fontdir(fs::path(PlatformUtils::resourcesPath()) / "fonts");
	if (fs::is_regular_file(fontdir / "fonts.conf")) {
		PlatformUtils::setenv("FONTCONFIG_PATH", boosty::stringy(boosty::absolute(fontdir)).c_str(), 0);
	}

	// Just load the configs. We'll build the fonts once all configs are loaded
	config = FcInitLoadConfig();
	if (!config) {
		PRINT("Can't initialize fontconfig library, text() objects will not be rendered");
		return;
	}

	// Add the built-in fonts & config
	fs::path builtinfontpath = fs::path(PlatformUtils::resourcesPath()) / "fonts";
	if (fs::is_directory(builtinfontpath)) {
		add_font_dir(boosty::stringy(boosty::canonical(builtinfontpath)));
		FcConfigParseAndLoad(config, reinterpret_cast<const FcChar8 *>(boosty::stringy(builtinfontpath).c_str()), false);
	}

	const char *home = getenv("HOME");

#ifdef WIN32
	// Add Window font folders.
	const char *windir = getenv("WinDir");
	if (windir) {
		add_font_dir(std::string(windir) + "\\Fonts");
	}
#endif

	// Add Linux font folders, the system folders are expected to be
	// configured by the system configuration for fontconfig.
	if (home) {
		add_font_dir(std::string(home) + "/.fonts");
	}

	const char *env_font_path = getenv("OPENSCAD_FONT_PATH");
	if (env_font_path != NULL) {
		std::string paths(env_font_path);
		const std::string sep = PlatformUtils::pathSeparatorChar();
		typedef boost::split_iterator<std::string::iterator> string_split_iterator;
		for (string_split_iterator it = boost::make_split_iterator(paths, boost::first_finder(sep, boost::is_iequal())); it != string_split_iterator(); it++) {
			const fs::path p(boost::copy_range<std::string>(*it));
			if (fs::exists(p) && fs::is_directory(p)) {
				std::string path = boosty::absolute(p).string();
				add_font_dir(path);
			}
		}
	}

	// FIXME: Caching happens here. This would be a good place to notify the user
	FcConfigBuildFonts(config);

	// For use by LibraryInfo
	FcStrList *dirs = FcConfigGetFontDirs(config);
	while (FcChar8 *dir = FcStrListNext(dirs)) {
		fontpath.push_back(std::string((const char *)dir));
	}
	FcStrListDone(dirs);

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

void FontCache::register_font_file(const std::string path)
{
	if (!FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8 *> (path.c_str()))) {
		PRINTB("Can't register font '%s'", path);
	}
}

void FontCache::add_font_dir(const std::string path)
{
	if (!fs::is_directory(path)) {
		return;
	}
	if (!FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8 *> (path.c_str()))) {
		PRINTB("Can't register font directory '%s'", path);
	}
}

FontInfoList * FontCache::list_fonts()
{
	FcObjectSet *object_set = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, (char *) 0);
	FcPattern *pattern = FcPatternCreate();
	init_pattern(pattern);
	FcFontSet *font_set = FcFontList(config, pattern, object_set);
	FcObjectSetDestroy(object_set);
	FcPatternDestroy(pattern);

	FontInfoList *list = new FontInfoList();
	for (int a = 0; a < font_set->nfont; a++) {
		FcValue file_value;
		FcPatternGet(font_set->fonts[a], FC_FILE, 0, &file_value);

		FcValue family_value;
		FcPatternGet(font_set->fonts[a], FC_FAMILY, 0, &family_value);

		FcValue style_value;
		FcPatternGet(font_set->fonts[a], FC_STYLE, 0, &style_value);

		std::string family((const char *) family_value.u.s);
		std::string style((const char *) style_value.u.s);
		std::string file((const char *) file_value.u.s);

		list->push_back(FontInfo(family, style, file));
	}
	FcFontSetDestroy(font_set);

	return list;
}

bool FontCache::is_init_ok()
{
	return init_ok;
}

void FontCache::clear()
{
	cache.clear();
}

void FontCache::dump_cache(const std::string info)
{
	std::cout << info << ":";
	for (cache_t::iterator it = cache.begin(); it != cache.end(); it++) {
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
	for (cache_t::iterator it = cache.begin(); it != cache.end(); it++) {
		if ((*pos).second.second > (*it).second.second) {
			pos = it;
		}
	}
	FT_Done_Face((*pos).second.first);
	cache.erase(pos);
}

FT_Face FontCache::get_font(const std::string font)
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

FT_Face FontCache::find_face(const std::string font)
{
	std::string trimmed(font);
	boost::algorithm::trim(trimmed);

	const std::string lookup = trimmed.empty() ? DEFAULT_FONT : trimmed;
	PRINTDB("font = \"%s\", lookup = \"%s\"", font % lookup);
	FT_Face face = find_face_fontconfig(lookup);
	PRINTDB("result = \"%s\", style = \"%s\"", face->family_name % face->style_name);
	return face;
}

void FontCache::init_pattern(FcPattern *pattern)
{
	FcValue true_value;
	true_value.type = FcTypeBool;
	true_value.u.b = true;

	FcPatternAdd(pattern, FC_OUTLINE, true_value, true);
	FcPatternAdd(pattern, FC_SCALABLE, true_value, true);
}

FT_Face FontCache::find_face_fontconfig(const std::string font)
{
	FcResult result;

	FcPattern *pattern = FcNameParse((unsigned char *) font.c_str());
	init_pattern(pattern);

	FcDefaultSubstitute(pattern);
	FcConfigSubstitute(config, pattern, FcMatchFont);
	FcPattern *match = FcFontMatch(config, pattern, &result);

	FcValue file_value;
	if (FcPatternGet(match, FC_FILE, 0, &file_value) != FcResultMatch) {
		return NULL;
	}

	FcValue font_index;
	if (FcPatternGet(match, FC_INDEX, 0, &font_index)) {
		return NULL;
	}
	
	FT_Face face;
	FT_Error error = FT_New_Face(library, (const char *) file_value.u.s, font_index.u.i, &face);

	FcPatternDestroy(pattern);
	FcPatternDestroy(match);

	for (int a = 0; a < face->num_charmaps; a++) {
		FT_CharMap charmap = face->charmaps[a];
		PRINTDB("charmap = %d: platform = %d, encoding = %d", a % charmap->platform_id % charmap->encoding_id);
	}

	if (FT_Select_Charmap(face, ft_encoding_unicode) == 0) {
		PRINTDB("Successfully selected unicode charmap: %s/%s", face->family_name % face->style_name);
	} else {
		bool charmap_set = false;
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS);
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_ISO, TT_ISO_ID_10646);
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_APPLE_UNICODE, -1);
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_MICROSOFT, TT_MS_ID_SYMBOL_CS);
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN);
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_ISO, TT_ISO_ID_8859_1);
		if (!charmap_set)
			charmap_set = try_charmap(face, TT_PLATFORM_ISO, TT_ISO_ID_7BIT_ASCII);
		if (!charmap_set)
			PRINTB("Warning: Could not select a char map for font %s/%s", face->family_name % face->style_name);
	}
	
	return error ? NULL : face;
}

bool FontCache::try_charmap(FT_Face face, int platform_id, int encoding_id)
{
	for (int idx = 0; idx < face->num_charmaps; idx++) {
		FT_CharMap charmap = face->charmaps[idx];
		if ((charmap->platform_id == platform_id) && ((encoding_id < 0) || (charmap->encoding_id == encoding_id))) {
			if (FT_Set_Charmap(face, charmap) == 0) {
				PRINTDB("Selected charmap: platform_id = %d, encoding_id = %d", charmap->platform_id % charmap->encoding_id);
				if (is_windows_symbol_font(face)) {
					PRINTDB("Detected windows symbol font with character codes in the Private Use Area of Unicode at 0xf000: %s/%s", face->family_name % face->style_name);
				}
				return true;
			}
		}
	}
	return false;
}

bool FontCache::is_windows_symbol_font(FT_Face face)
{
	if (face->charmap->platform_id != TT_PLATFORM_MICROSOFT) {
		return false;
	}

	if (face->charmap->encoding_id != TT_MS_ID_SYMBOL_CS) {
		return false;
	}

	FT_UInt gindex;
	FT_ULong charcode = FT_Get_First_Char(face, &gindex);
	if ((gindex == 0) || (charcode < 0xf000)) {
		return false;
	}
	
	return true;
}

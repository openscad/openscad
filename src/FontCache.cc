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
#include "PlatformUtils.h"
#include "parsersettings.h"

extern std::vector<std::string> librarypath;

std::vector<std::string> fontpath;

namespace fs = boost::filesystem;

static bool FontInfoSortPredicate(const FontInfo& fi1, const FontInfo& fi2)
{
	return (fi1 < fi2);
}

FontInfo::FontInfo(const std::string &family, const std::string &style, const std::string &file) : family(family), style(style), file(file)
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

const std::string &FontInfo::get_family() const
{
	return family;
}

const std::string &FontInfo::get_style() const
{
	return style;
}

const std::string &FontInfo::get_file() const
{
	return file;
}

FontCache * FontCache::self = nullptr;
FontCache::InitHandlerFunc *FontCache::cb_handler = FontCache::defaultInitHandler;
void *FontCache::cb_userdata = nullptr;
const std::string FontCache::DEFAULT_FONT("Liberation Sans:style=Regular");

/**
 * Default implementation for the font cache initialization. In case no other
 * handler is registered, the cache build is just called synchronously in the
 * current thread by this handler.
 */
void FontCache::defaultInitHandler(FontCacheInitializer *initializer, void *)
{
	initializer->run();
}

FontCache::FontCache()
{
	this->init_ok = false;

	// If we've got a bundled fonts.conf, initialize fontconfig with our own config
	// by overriding the built-in fontconfig path.
	// For system installs and dev environments, we leave this alone
	fs::path fontdir(PlatformUtils::resourcePath("fonts"));
	if (fs::is_regular_file(fontdir / "fonts.conf")) {
		PlatformUtils::setenv("FONTCONFIG_PATH", (fs::absolute(fontdir).generic_string()).c_str(), 0);
	}

	// Just load the configs. We'll build the fonts once all configs are loaded
	this->config = FcInitLoadConfig();
	if (!this->config) {
		PRINT("WARNING: Can't initialize fontconfig library, text() objects will not be rendered");
		return;
	}

	// Add the built-in fonts & config
	fs::path builtinfontpath(PlatformUtils::resourcePath("fonts"));
	if (fs::is_directory(builtinfontpath)) {
		FcConfigParseAndLoad(this->config, reinterpret_cast<const FcChar8 *>(builtinfontpath.generic_string().c_str()), false);
		add_font_dir(boosty::canonical(builtinfontpath).generic_string());
	}

	const char *home = getenv("HOME");

	// Add Linux font folders, the system folders are expected to be
	// configured by the system configuration for fontconfig.
	if (home) {
		add_font_dir(std::string(home) + "/.fonts");
	}

	const char *env_font_path = getenv("OPENSCAD_FONT_PATH");
	if (env_font_path != nullptr) {
		std::string paths(env_font_path);
		const std::string sep = PlatformUtils::pathSeparatorChar();
		typedef boost::split_iterator<std::string::iterator> string_split_iterator;
		for (string_split_iterator it = boost::make_split_iterator(paths, boost::first_finder(sep, boost::is_iequal())); it != string_split_iterator(); it++) {
			const fs::path p(boost::copy_range<std::string>(*it));
			if (fs::exists(p) && fs::is_directory(p)) {
				std::string path = fs::absolute(p).string();
				add_font_dir(path);
			}
		}
	}

	FontCacheInitializer initializer(this->config);
	cb_handler(&initializer, cb_userdata);

	// For use by LibraryInfo
	FcStrList *dirs = FcConfigGetFontDirs(this->config);
	while (FcChar8 *dir = FcStrListNext(dirs)) {
		fontpath.push_back(std::string((const char *)dir));
	}
	FcStrListDone(dirs);

	const FT_Error error = FT_Init_FreeType(&this->library);
	if (error) {
		PRINT("WARNING: Can't initialize freetype library, text() objects will not be rendered");
		return;
	}

	this->init_ok = true;
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

void FontCache::registerProgressHandler(InitHandlerFunc *handler, void *userdata)
{
	FontCache::cb_handler = handler;
	FontCache::cb_userdata = userdata;
}

void FontCache::register_font_file(const std::string &path)
{
	if (!FcConfigAppFontAddFile(this->config, reinterpret_cast<const FcChar8 *> (path.c_str()))) {
		PRINTB("Can't register font '%s'", path);
	}
}

void FontCache::add_font_dir(const std::string &path)
{
	if (!fs::is_directory(path)) {
		return;
	}
	if (!FcConfigAppFontAddDir(this->config, reinterpret_cast<const FcChar8 *> (path.c_str()))) {
		PRINTB("Can't register font directory '%s'", path);
	}
}

FontInfoList *FontCache::list_fonts() const
{
	FcObjectSet *object_set = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, (char *) 0);
	FcPattern *pattern = FcPatternCreate();
	init_pattern(pattern);
	FcFontSet *font_set = FcFontList(this->config, pattern, object_set);
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

bool FontCache::is_init_ok() const
{
	return this->init_ok;
}

void FontCache::clear()
{
	this->cache.clear();
}

void FontCache::dump_cache(const std::string &info)
{
	std::cout << info << ":";
	for (cache_t::iterator it = this->cache.begin(); it != this->cache.end(); it++) {
		std::cout << " " << (*it).first << " (" << (*it).second.second << ")";
	}
	std::cout << std::endl;
}

void FontCache::check_cleanup()
{
	if (this->cache.size() < MAX_NR_OF_CACHE_ENTRIES) {
		return;
	}

	cache_t::iterator pos = this->cache.begin()++;
	for (cache_t::iterator it = this->cache.begin(); it != this->cache.end(); it++) {
		if ((*pos).second.second > (*it).second.second) {
			pos = it;
		}
	}
	FT_Done_Face((*pos).second.first);
	this->cache.erase(pos);
}

FT_Face FontCache::get_font(const std::string &font)
{
	FT_Face face;
	cache_t::iterator it = this->cache.find(font);
	if (it == this->cache.end()) {
		face = find_face(font);
		if (!face) {
			return nullptr;
		}
		check_cleanup();
	} else {
		face = (*it).second.first;
	}
	this->cache[font] = cache_entry_t(face, time(nullptr));
	return face;
}

FT_Face FontCache::find_face(const std::string &font) const
{
	std::string trimmed(font);
	boost::algorithm::trim(trimmed);

	const std::string lookup = trimmed.empty() ? DEFAULT_FONT : trimmed;
	PRINTDB("font = \"%s\", lookup = \"%s\"", font % lookup);
	FT_Face face = find_face_fontconfig(lookup);
	PRINTDB("result = \"%s\", style = \"%s\"", face->family_name % face->style_name);
	return face;
}

void FontCache::init_pattern(FcPattern *pattern) const
{
	FcValue true_value;
	true_value.type = FcTypeBool;
	true_value.u.b = true;

	FcPatternAdd(pattern, FC_OUTLINE, true_value, true);
	FcPatternAdd(pattern, FC_SCALABLE, true_value, true);
}

FT_Face FontCache::find_face_fontconfig(const std::string &font) const
{
	FcResult result;

	FcPattern *pattern = FcNameParse((unsigned char *)font.c_str());
	init_pattern(pattern);

	FcConfigSubstitute(this->config, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	FcPattern *match = FcFontMatch(this->config, pattern, &result);

	FcValue file_value;
	if (FcPatternGet(match, FC_FILE, 0, &file_value) != FcResultMatch) {
		return nullptr;
	}

	FcValue font_index;
	if (FcPatternGet(match, FC_INDEX, 0, &font_index)) {
		return nullptr;
	}
	
	FT_Face face;
	FT_Error error = FT_New_Face(this->library, (const char *) file_value.u.s, font_index.u.i, &face);

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
	
	return error ? nullptr : face;
}

bool FontCache::try_charmap(FT_Face face, int platform_id, int encoding_id) const
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

bool FontCache::is_windows_symbol_font(const FT_Face &face) const
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

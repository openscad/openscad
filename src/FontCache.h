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
#pragma once

#include <map>
#include <string>
#include <iostream>

#include <time.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_IDS_H

#include <vector>
#include <string>
#include <fontconfig/fontconfig.h>

#include <hb.h>
#include <hb-ft.h>

class FontInfo
{
public:
	FontInfo(const std::string &family, const std::string &style, const std::string &file);
	virtual ~FontInfo();

	const std::string &get_family() const;
	const std::string &get_style() const;
	const std::string &get_file() const;
	bool operator<(const FontInfo &rhs) const;
private:
	std::string family;
	std::string style;
	std::string file;
};

typedef std::vector<FontInfo> FontInfoList;

/**
 * Slow call of the font cache initialization. This is separated here so it
 * can be passed to the GUI to run in a separate thread while showing a
 * progress dialog.
 */
class FontCacheInitializer
{
public:
	FontCacheInitializer(FcConfig *config) : config(config) { }
	void run() { FcConfigBuildFonts(config); }
private:
	FcConfig *config;
};

class FontCache
{
public:
	const static std::string DEFAULT_FONT;
	const static unsigned int MAX_NR_OF_CACHE_ENTRIES = 3;

	FontCache();
	virtual ~FontCache();

	bool is_init_ok() const;
	FT_Face get_font(const std::string &font);
	bool is_windows_symbol_font(const FT_Face &face) const;
	void register_font_file(const std::string &path);
	void clear();
	FontInfoList *list_fonts() const;

	static FontCache *instance();

	typedef void (InitHandlerFunc)(FontCacheInitializer *initializer, void *userdata);
	static void registerProgressHandler(InitHandlerFunc *handler, void *userdata = nullptr);

private:
	typedef std::pair<FT_Face, time_t> cache_entry_t;
	typedef std::map<std::string, cache_entry_t> cache_t;

	static FontCache *self;
	static InitHandlerFunc *cb_handler;
	static void *cb_userdata;

	static void defaultInitHandler(FontCacheInitializer *delegate, void *userdata);

	bool init_ok;
	cache_t cache;
	FcConfig *config;
	FT_Library library;

	void check_cleanup();
	void dump_cache(const std::string &info);

	void add_font_dir(const std::string &path);
	void init_pattern(FcPattern *pattern) const;

	FT_Face find_face(const std::string &font) const;
	FT_Face find_face_fontconfig(const std::string &font) const;
	bool try_charmap(FT_Face face, int platform_id, int encoding_id) const;
};


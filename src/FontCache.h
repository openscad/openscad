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

#include <vector>
#include <string>
#include <fontconfig/fontconfig.h>

#include <hb.h>
#include <hb-ft.h>

class FontInfo {
public:
    FontInfo(std::string family, std::string style, std::string file);
    virtual ~FontInfo();
    
    std::string get_family() const;
    std::string get_style() const;
    std::string get_file() const;
    bool operator<(const FontInfo &rhs) const;
private:
    std::string family;
    std::string style;
    std::string file;
};

typedef std::vector<FontInfo> FontInfoList;

class FontCache {
public:
    const static std::string DEFAULT_FONT;
    const static unsigned int MAX_NR_OF_CACHE_ENTRIES = 3;
    
    FontCache();
    virtual ~FontCache();

    bool is_init_ok();
    FT_Face get_font(std::string font);
    void register_font_file(std::string path);
    void clear();
    FontInfoList * list_fonts();
    
    static FontCache * instance();
private:
    typedef std::pair<FT_Face, time_t> cache_entry_t;
    typedef std::map<std::string, cache_entry_t> cache_t;

    static FontCache *self;
    
    bool init_ok;
    cache_t cache;
    FcConfig *config;
    FT_Library library;

    void check_cleanup();
    void dump_cache(const std::string info);
    
    void add_font_dir(const std::string path);
    void init_pattern(FcPattern *pattern);
    
    FT_Face find_face(const std::string font);
    FT_Face find_face_fontconfig(const std::string font);
    FT_Face find_face_in_path_list(const std::string font);
    FT_Face find_face_in_path(const std::string path, const std::string font);
};


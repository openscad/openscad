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

#include <string>
#include <vector>
#include <ostream>

#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

class FreetypeRenderer
{
public:
	class Params
	{
public:
		void set_size(double size) {
			this->size = size;
		}
		void set_spacing(double spacing) {
			this->spacing = spacing;
		}
		void set_fn(double fn) {
			this->fn = fn;
		}
		void set_fa(double fa) {
			this->fa = fa;
		}
		void set_fs(double fs) {
			this->fs = fs;
		}
		void set_segments(double segments) {
			this->segments = segments;
		}
		void set_text(std::string text) {
			this->text = text;
		}
		void set_font(std::string font) {
			this->font = font;
		}
		void set_direction(std::string direction) {
			this->direction = direction;
		}
		void set_language(std::string language) {
			this->language = language;
		}
		void set_script(std::string script) {
			this->script = script;
		}
		void set_halign(std::string halign) {
			this->halign = halign;
		}
		void set_valign(std::string valign) {
			this->valign = valign;
		}
		friend std ::ostream &operator<<(std::ostream &stream, const FreetypeRenderer::Params &params) {
			return stream
						 << "text = \"" << params.text
						 << "\", size = " << params.size
						 << ", spacing = " << params.spacing
						 << ", font = \"" << params.font
						 << "\", direction = \"" << params.direction
						 << "\", language = \"" << params.language
						 << (params.script.empty() ? "" : "\", script = \"") << params.script
						 << "\", halign = \"" << params.halign
						 << "\", valign = \"" << params.valign
						 << "\", $fn = " << params.fn
						 << ", $fa = " << params.fa
						 << ", $fs = " << params.fs;
		}
private:
		double size, spacing, fn, fa, fs, segments;
		std::string text, font, direction, language, script, halign, valign;

		friend class FreetypeRenderer;
	};

	FreetypeRenderer();
	virtual ~FreetypeRenderer();

	void detect_properties(FreetypeRenderer::Params &params) const;
	std::vector<const class Geometry *> render(const FreetypeRenderer::Params &params) const;
private:
	const static double scale;
	FT_Outline_Funcs funcs;

	class GlyphData
	{
public:
		GlyphData(FT_Glyph glyph, unsigned int idx, hb_glyph_position_t *glyph_pos) : glyph(glyph), idx(idx), glyph_pos(glyph_pos) {}
		unsigned int get_idx() const { return idx; }
		FT_Glyph get_glyph() const { return glyph; }
		double get_x_offset() const { return glyph_pos->x_offset / 64.0 / 16.0; }
		double get_y_offset() const { return glyph_pos->y_offset / 64.0 / 16.0; }
		double get_x_advance() const { return glyph_pos->x_advance / 64.0 / 16.0; }
		double get_y_advance() const { return glyph_pos->y_advance / 64.0 / 16.0; }
private:
		FT_Glyph glyph;
		unsigned int idx;
		hb_glyph_position_t *glyph_pos;
	};

	struct done_glyph : public std::unary_function<const GlyphData *, void> {
		void operator()(const GlyphData *glyph_data) {
			FT_Done_Glyph(glyph_data->get_glyph());
			delete glyph_data;
		}
	};

	class GlyphArray : public std::vector<const GlyphData *>
	{
public:
		virtual ~GlyphArray() {
			std::for_each(begin(), end(), done_glyph());
		}
	};

	bool is_ignored_script(const hb_script_t script) const;
	hb_script_t get_script(const FreetypeRenderer::Params &params, hb_glyph_info_t *glyph_info, unsigned int glyph_count) const;
	hb_direction_t get_direction(const FreetypeRenderer::Params &params, const hb_script_t script) const;

	double calc_x_offset(std::string halign, double width) const;
	double calc_y_offset(std::string valign, double ascend, double descend) const;

	static int outline_move_to_func(const FT_Vector *to, void *user);
	static int outline_line_to_func(const FT_Vector *to, void *user);
	static int outline_conic_to_func(const FT_Vector *c1, const FT_Vector *to, void *user);
	static int outline_cubic_to_func(const FT_Vector *c1, const FT_Vector *c2, const FT_Vector *to, void *user);
};

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
#include <math.h>
#include <stdio.h>

#include <iostream>

#include <glib.h>

#include <fontconfig/fontconfig.h>

#include "printutils.h"

#include "FontCache.h"
#include "DrawingCallback.h"
#include "FreetypeRenderer.h"

#include FT_OUTLINE_H

#define SCRIPT_UNTAG(tag)   ((uint8_t)((tag) >> 24)) % ((uint8_t)((tag) >> 16)) % ((uint8_t)((tag) >> 8)) % ((uint8_t)(tag))

static inline Vector2d get_scaled_vector(const FT_Vector *ft_vector, double scale) {
	return Vector2d(ft_vector->x / scale, ft_vector->y / scale);
}

const double FreetypeRenderer::scale = 1000;

FreetypeRenderer::FreetypeRenderer()
{
	funcs.move_to = outline_move_to_func;
	funcs.line_to = outline_line_to_func;
	funcs.conic_to = outline_conic_to_func;
	funcs.cubic_to = outline_cubic_to_func;
	funcs.delta = 0;
	funcs.shift = 0;
}

FreetypeRenderer::~FreetypeRenderer()
{
}

int FreetypeRenderer::outline_move_to_func(const FT_Vector *to, void *user)
{
	DrawingCallback *cb = reinterpret_cast<DrawingCallback *>(user);

	cb->move_to(get_scaled_vector(to, scale));
	return 0;
}

int FreetypeRenderer::outline_line_to_func(const FT_Vector *to, void *user)
{
	DrawingCallback *cb = reinterpret_cast<DrawingCallback *>(user);

	cb->line_to(get_scaled_vector(to, scale));
	return 0;
}

int FreetypeRenderer::outline_conic_to_func(const FT_Vector *c1, const FT_Vector *to, void *user)
{
	DrawingCallback *cb = reinterpret_cast<DrawingCallback *>(user);

	cb->curve_to(get_scaled_vector(c1, scale), get_scaled_vector(to, scale));
	return 0;
}

int FreetypeRenderer::outline_cubic_to_func(const FT_Vector *c1, const FT_Vector *c2, const FT_Vector *to, void *user)
{
	DrawingCallback *cb = reinterpret_cast<DrawingCallback *>(user);

	cb->curve_to(get_scaled_vector(c1, scale), get_scaled_vector(c2, scale), get_scaled_vector(to, scale));
	return 0;
}

double FreetypeRenderer::calc_x_offset(std::string halign, double width) const
{
	if (halign == "right") {
		return -width;
	}
	else if (halign == "center") {
		return -width / 2.0;
	}
	else {
		if (halign != "left") {
			PRINTB("Unknown value for the halign parameter (use \"left\", \"right\" or \"center\"): '%s'", halign);
		}
		return 0;
	}
}

double FreetypeRenderer::calc_y_offset(std::string valign, double ascend, double descend) const
{
	if (valign == "top") {
		return -ascend;
	}
	else if (valign == "center") {
		return descend / 2.0 - ascend / 2.0;
	}
	else if (valign == "bottom") {
		return descend;
	}
	else {
		if (valign != "baseline") {
			PRINTB("Unknown value for the valign parameter (use \"baseline\", \"bottom\", \"top\" or \"center\"): '%s'", valign);
		}
		return 0;
	}
}

hb_direction_t FreetypeRenderer::get_direction(const FreetypeRenderer::Params &params, const hb_script_t script) const
{
	hb_direction_t param_direction = hb_direction_from_string(params.direction.c_str(), -1);
	if (param_direction != HB_DIRECTION_INVALID) {
		return param_direction;
	}

	hb_direction_t direction = hb_script_get_horizontal_direction(script);
	PRINTDB("Detected direction '%s' for %s", hb_direction_to_string(direction) % params.text.c_str());
	return direction;
}

bool FreetypeRenderer::is_ignored_script(const hb_script_t script) const
{
	switch (script) {
	case HB_SCRIPT_COMMON:
	case HB_SCRIPT_INHERITED:
	case HB_SCRIPT_UNKNOWN:
	case HB_SCRIPT_INVALID:
		return true;
	default:
		return false;
	}
}

hb_script_t FreetypeRenderer::get_script(const FreetypeRenderer::Params &params, hb_glyph_info_t *glyph_info, unsigned int glyph_count) const
{
	hb_script_t param_script = hb_script_from_string(params.script.c_str(), -1);
	if (param_script != HB_SCRIPT_INVALID) {
		return param_script;
	}

	hb_script_t script = HB_SCRIPT_INVALID;
	for (unsigned int idx = 0; idx < glyph_count; idx++) {
		hb_codepoint_t cp = glyph_info[idx].codepoint;
		hb_script_t s = hb_unicode_script(hb_unicode_funcs_get_default(), cp);
		if (!is_ignored_script(s)) {
			if (script == HB_SCRIPT_INVALID) {
				script = s;
			}
			else if ((script != s) && (script != HB_SCRIPT_UNKNOWN)) {
				script = HB_SCRIPT_UNKNOWN;
			}
		}
	}
	PRINTDB("Detected script '%c%c%c%c' for %s", SCRIPT_UNTAG(script) % params.text.c_str());
	return script;
}

void FreetypeRenderer::detect_properties(FreetypeRenderer::Params &params) const
{
	hb_buffer_t *hb_buf = hb_buffer_create();
	hb_buffer_add_utf8(hb_buf, params.text.c_str(), strlen(params.text.c_str()), 0, strlen(params.text.c_str()));

	unsigned int glyph_count;
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);

	hb_script_t script = get_script(params, glyph_info, glyph_count);
	hb_buffer_destroy(hb_buf);

	if (!is_ignored_script(script)) {
		char script_buf[5] = { 0, };
		hb_tag_to_string(hb_script_to_iso15924_tag(script), script_buf);
		params.set_script(script_buf);
	}

	hb_direction_t direction = get_direction(params, script);
	params.set_direction(hb_direction_to_string(direction));
}

std::vector<const Geometry *> FreetypeRenderer::render(const FreetypeRenderer::Params &params) const
{
	FT_Face face;
	FT_Error error;
	DrawingCallback callback(params.segments);

	FontCache *cache = FontCache::instance();
	if (!cache->is_init_ok()) {
		return std::vector<const Geometry *>();
	}

	face = cache->get_font(params.font);
	if (face == nullptr) {
		return std::vector<const Geometry *>();
	}

	error = FT_Set_Char_Size(face, 0, params.size * scale, 100, 100);
	if (error) {
		PRINTB("Can't set font size for font %s", params.font);
		return std::vector<const Geometry *>();
	}

	hb_font_t *hb_ft_font = hb_ft_font_create(face, nullptr);

	hb_buffer_t *hb_buf = hb_buffer_create();
	hb_buffer_set_direction(hb_buf, hb_direction_from_string(params.direction.c_str(), -1));
	hb_buffer_set_script(hb_buf, hb_script_from_string(params.script.c_str(), -1));
	hb_buffer_set_language(hb_buf, hb_language_from_string(params.language.c_str(), -1));
	if (FontCache::instance()->is_windows_symbol_font(face)) {
		// Special handling for symbol fonts like Webdings.
		// see http://www.microsoft.com/typography/otspec/recom.htm
		//
		// We go through the string char by char and if the codepoint
		// value is between 0x00 and 0xff, then the codepoint is translated
		// to the 0xf000 page (Private Use Area of Unicode). All other
		// values are untouched, so using the correct codepoint directly
		// (e.g. \uf021 for the spider in Webdings) still works.
		const char *p = params.text.c_str();
		if (g_utf8_validate(p, -1, nullptr)) {
			char buf[8];
			while (*p != 0) {
				memset(buf, 0, 8);
				gunichar c = g_utf8_get_char(p);
				c = (c < 0x0100) ? 0xf000 + c : c;
				g_unichar_to_utf8(c, buf);
				hb_buffer_add_utf8(hb_buf, buf, strlen(buf), 0, strlen(buf));
				p = g_utf8_next_char(p);
			}
		}
		else {
			PRINTB("Warning: Ignoring text with invalid UTF-8 encoding: \"%s\"", params.text.c_str());
		}
	}
	else {
		hb_buffer_add_utf8(hb_buf, params.text.c_str(), strlen(params.text.c_str()), 0, strlen(params.text.c_str()));
	}
	hb_shape(hb_ft_font, hb_buf, nullptr, 0);

	unsigned int glyph_count;
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);

	GlyphArray glyph_array;
	for (unsigned int idx = 0; idx < glyph_count; idx++) {
		FT_UInt glyph_index = glyph_info[idx].codepoint;
		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		if (error) {
			PRINTB("Could not load glyph %u for char at index %u in text '%s'", glyph_index % idx % params.text);
			continue;
		}

		FT_Glyph glyph;
		error = FT_Get_Glyph(face->glyph, &glyph);
		if (error) {
			PRINTB("Could not get glyph %u for char at index %u in text '%s'", glyph_index % idx % params.text);
			continue;
		}
		const GlyphData *glyph_data = new GlyphData(glyph, idx, &glyph_pos[idx]);
		glyph_array.push_back(glyph_data);
	}

	double width = 0, ascend = 0, descend = 0;
	for (GlyphArray::iterator it = glyph_array.begin(); it != glyph_array.end(); it++) {
		const GlyphData *glyph = (*it);

		FT_BBox bbox;
		FT_Glyph_Get_CBox(glyph->get_glyph(), FT_GLYPH_BBOX_GRIDFIT, &bbox);

		if (HB_DIRECTION_IS_HORIZONTAL(hb_buffer_get_direction(hb_buf))) {
			double asc = std::max(0.0, bbox.yMax / 64.0 / 16.0);
			double desc = std::max(0.0, -bbox.yMin / 64.0 / 16.0);
			width += glyph->get_x_advance() * params.spacing;
			ascend = std::max(ascend, asc);
			descend = std::max(descend, desc);
		}
		else {
			double w_bbox = (bbox.xMax - bbox.xMin) / 64.0 / 16.0;
			width = std::max(width, w_bbox);
			ascend += glyph->get_y_advance() * params.spacing;
		}
	}

	double x_offset = calc_x_offset(params.halign, width);
	double y_offset = calc_y_offset(params.valign, ascend, descend);

	for (GlyphArray::iterator it = glyph_array.begin(); it != glyph_array.end(); it++) {
		const GlyphData *glyph = (*it);

		callback.start_glyph();
		callback.set_glyph_offset(x_offset + glyph->get_x_offset(), y_offset + glyph->get_y_offset());
		FT_Outline outline = reinterpret_cast<FT_OutlineGlyph>(glyph->get_glyph())->outline;
		FT_Outline_Decompose(&outline, &funcs, &callback);

		double adv_x = glyph->get_x_advance() * params.spacing;
		double adv_y = glyph->get_y_advance() * params.spacing;
		callback.add_glyph_advance(adv_x, adv_y);
		callback.finish_glyph();
	}

	hb_buffer_destroy(hb_buf);
	hb_font_destroy(hb_ft_font);

	return callback.get_result();
}

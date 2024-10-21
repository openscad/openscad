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
#include "core/FreetypeRenderer.h"

#include <algorithm>
#include <limits>
#include <cstdint>
#include <memory>
#include <cmath>
#include <cstdio>
#include <vector>


#include <fontconfig/fontconfig.h>

#include "utils/printutils.h"

#include "FontCache.h"
#include "core/DrawingCallback.h"
#include "utils/calc.h"

#include FT_OUTLINE_H
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define SCRIPT_UNTAG(tag)   ((uint8_t)((tag) >> 24)) % ((uint8_t)((tag) >> 16)) % ((uint8_t)((tag) >> 8)) % ((uint8_t)(tag))

static inline Vector2d get_scaled_vector(const FT_Vector *ft_vector, double scale) {
  return {ft_vector->x / scale, ft_vector->y / scale};
}

const double FreetypeRenderer::scale = 1e5;

FreetypeRenderer::FreetypeRenderer()
{
  funcs.move_to = outline_move_to_func;
  funcs.line_to = outline_line_to_func;
  funcs.conic_to = outline_conic_to_func;
  funcs.cubic_to = outline_cubic_to_func;
  funcs.delta = 0;
  funcs.shift = 0;
}

int FreetypeRenderer::outline_move_to_func(const FT_Vector *to, void *user)
{
  auto *cb = reinterpret_cast<DrawingCallback *>(user);

  cb->move_to(get_scaled_vector(to, scale));
  return 0;
}

int FreetypeRenderer::outline_line_to_func(const FT_Vector *to, void *user)
{
  auto *cb = reinterpret_cast<DrawingCallback *>(user);

  cb->line_to(get_scaled_vector(to, scale));
  return 0;
}

int FreetypeRenderer::outline_conic_to_func(const FT_Vector *c1, const FT_Vector *to, void *user)
{
  auto *cb = reinterpret_cast<DrawingCallback *>(user);

  cb->curve_to(get_scaled_vector(c1, scale), get_scaled_vector(to, scale));
  return 0;
}

int FreetypeRenderer::outline_cubic_to_func(const FT_Vector *c1, const FT_Vector *c2, const FT_Vector *to, void *user)
{
  auto *cb = reinterpret_cast<DrawingCallback *>(user);

  cb->curve_to(get_scaled_vector(c1, scale), get_scaled_vector(c2, scale), get_scaled_vector(to, scale));
  return 0;
}

// Calculate offsets for horizontal text.
void FreetypeRenderer::ShapeResults::calc_offsets_horiz(
  const FreetypeRenderer::Params& params)
{
  if (params.halign == "right") {
    x_offset = -advance_x;
  } else if (params.halign == "center") {
    x_offset = -advance_x / 2.0;
  } else if (params.halign == "left" || params.halign == "default") {
    x_offset = 0;
  } else {
    LOG(message_group::Warning, params.loc, params.documentPath,
        "Unknown value for the halign parameter"
        " (use \"left\", \"right\" or \"center\"): '%1$s'",
        params.halign);
    x_offset = 0;
  }

  if (params.valign == "top") {
    y_offset = -ascent;
  } else if (params.valign == "center") {
    double height = ascent - descent;
    y_offset = -height / 2 - descent;
  } else if (params.valign == "bottom") {
    y_offset = -descent;
  } else if (params.valign == "baseline" || params.valign == "default") {
    y_offset = 0;
  } else {
    LOG(message_group::Warning, params.loc, params.documentPath,
        "Unknown value for the valign parameter"
        " (use \"baseline\", \"bottom\", \"top\" or \"center\"): '%1$s'",
        params.valign);
    y_offset = 0;
  }
}

// Calculate offsets for vertical text.
void FreetypeRenderer::ShapeResults::calc_offsets_vert(
  const FreetypeRenderer::Params& params)
{
  if (params.halign == "right") {
    x_offset = -right;
  } else if (params.halign == "left") {
    x_offset = -left;
  } else if (params.halign == "center" || params.halign == "default") {
    x_offset = 0;
  } else {
    LOG(message_group::Warning, params.loc, params.documentPath,
        "Unknown value for the halign parameter"
        " (use \"left\", \"right\" or \"center\"): '%1$s'",
        params.halign);
    x_offset = 0;
  }

  if (params.valign == "baseline") {
    LOG(message_group::Warning, params.loc, params.documentPath,
        "Don't use valign=\"baseline\" with vertical layouts",
        params.valign);
    y_offset = 0;
  } else if (params.valign == "center") {
    y_offset = -advance_y / 2.0;
  } else if (params.valign == "bottom") {
    y_offset = -advance_y;
  } else if (params.valign == "top" || params.valign == "default") {
    // Note that in vertical mode HarfBuzz sets the glyphs
    // below their origins, so this results in the entire string
    // being placed below the origin.
    y_offset = 0;
  } else {
    LOG(message_group::Warning, params.loc, params.documentPath,
        "Unknown value for the valign parameter"
        " (use \"baseline\", \"bottom\", \"top\" or \"center\"): '%1$s'",
        params.valign);
  }
}

hb_direction_t FreetypeRenderer::Params::detect_direction(const hb_script_t script) const
{
  hb_direction_t hbdirection;

  hbdirection = hb_direction_from_string(direction.c_str(), -1);
  if (hbdirection != HB_DIRECTION_INVALID) {
    PRINTDB("Explicit direction '%s' for %s",
            hb_direction_to_string(hbdirection) % text.c_str());
    return hbdirection;
  }

  hbdirection = hb_script_get_horizontal_direction(script);
  if (hbdirection != HB_DIRECTION_INVALID) {
    PRINTDB("Detected direction '%s' for %s",
            hb_direction_to_string(hbdirection) % text.c_str());
    return hbdirection;
  }

  PRINTDB("Unknown direction for %s; defaulting to LTR", text.c_str());
  return HB_DIRECTION_LTR;
}

bool FreetypeRenderer::Params::is_ignored_script(const hb_script_t script)
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

hb_script_t FreetypeRenderer::Params::detect_script(hb_glyph_info_t *glyph_info, unsigned int glyph_count) const
{
  hb_script_t hbscript;

  hbscript = hb_script_from_string(script.c_str(), -1);
  if (hbscript != HB_SCRIPT_INVALID) {
    return hbscript;
  }

  hbscript = HB_SCRIPT_INVALID;
  for (unsigned int idx = 0; idx < glyph_count; ++idx) {
    hb_codepoint_t cp = glyph_info[idx].codepoint;
    hb_script_t s = hb_unicode_script(hb_unicode_funcs_get_default(), cp);
    if (!is_ignored_script(s)) {
      if (hbscript == HB_SCRIPT_INVALID) {
        hbscript = s;
      } else if ((hbscript != s) && (hbscript != HB_SCRIPT_UNKNOWN)) {
        hbscript = HB_SCRIPT_UNKNOWN;
      }
    }
  }
  PRINTDB("Detected script '%c%c%c%c' for %s", SCRIPT_UNTAG(hbscript) % text.c_str());
  return hbscript;
}

void FreetypeRenderer::Params::detect_properties()
{
  hb_buffer_t *hb_buf = hb_buffer_create();
  hb_buffer_add_utf8(hb_buf, text.c_str(), strlen(text.c_str()), 0, strlen(text.c_str()));

  unsigned int glyph_count;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);

  hb_script_t hbscript = detect_script(glyph_info, glyph_count);
  hb_buffer_destroy(hb_buf);

  if (!is_ignored_script(hbscript)) {
    char script_buf[5] = { 0, };
    hb_tag_to_string(hb_script_to_iso15924_tag(hbscript), script_buf);
    set_script(script_buf);
  }

  hb_direction_t hbdirection = detect_direction(hbscript);
  set_direction(hb_direction_to_string(hbdirection));

  auto segments = Calc::get_fragments_from_r(size, fn, fs, fa);
  // The curved segments of most fonts are relatively short, so
  // by using a fraction of the number of full circle segments
  // the resolution will be better matching the detail level of
  // other objects.
  auto text_segments = std::max(segments / 8 + 1, 2);
  set_segments(text_segments);
}

FT_Face FreetypeRenderer::Params::get_font_face() const
{
  FontCache *cache = FontCache::instance();
  if (!cache->is_init_ok()) {
    LOG(message_group::Warning, loc, documentPath,
        "Font cache initialization failed");
    return nullptr;
  }

  FT_Face face = cache->get_font(font);
  if (face == nullptr) {
    LOG(message_group::Warning, loc, documentPath,
        "Can't get font %1$s", font);
    return nullptr;
  }

  FT_Error error = FT_Set_Char_Size(face, 0, scale, 100, 100);
  if (error) {
    LOG(message_group::Warning, loc, documentPath,
        "Can't set font size for font %1$s", font);
    return nullptr;
  }
  return face;
}

void FreetypeRenderer::Params::set(Parameters& parameters)
{
  // Note:
  // This populates all of the Params entries that text() populates.
  // Probably some of them are not needed by some callers.
  // However, we populate them here rather than "knowing" which
  // ones are and are not needed.

  (void) parameters.valid("size", Value::Type::NUMBER);
  (void) parameters.valid("text", Value::Type::STRING);
  (void) parameters.valid("spacing", Value::Type::NUMBER);
  (void) parameters.valid("font", Value::Type::STRING);
  (void) parameters.valid("direction", Value::Type::STRING);
  (void) parameters.valid("language", Value::Type::STRING);
  (void) parameters.valid("script", Value::Type::STRING);
  (void) parameters.valid("halign", Value::Type::STRING);
  (void) parameters.valid("valign", Value::Type::STRING);

  set_fn(parameters["$fn"].toDouble());
  set_fa(parameters["$fa"].toDouble());
  set_fs(parameters["$fs"].toDouble());

  set_size(parameters.get("size", 10.0));
  set_text(parameters.get("text", ""));
  set_spacing(parameters.get("spacing", 1.0));
  set_font(parameters.get("font", ""));
  set_direction(parameters.get("direction", ""));
  set_language(parameters.get("language", "en"));
  set_script(parameters.get("script", ""));
  set_halign(parameters.get("halign", "default"));
  set_valign(parameters.get("valign", "default"));
}


FreetypeRenderer::ShapeResults::ShapeResults(
  const FreetypeRenderer::Params& params)
{
  FT_Face face = params.get_font_face();
  if (face == nullptr) {
    return;
  }

  hb_ft_font = hb_ft_font_create(face, nullptr);

  hb_buf = hb_buffer_create();
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
    str_utf8_wrapper utf8_str{params.text};
    if (utf8_str.utf8_validate()) {
      for (auto ch : utf8_str) {
        gunichar c = ch.get_utf8_char();
        c = (c < 0x0100) ? 0xf000 + c : c;
        hb_buffer_add_utf32(hb_buf, &c, 1, 0, 1);
      }
    } else {
      LOG(message_group::Warning, params.loc, params.documentPath,
          "Ignoring text with invalid UTF-8 encoding: \"%1$s\"",
          params.text.c_str());
    }
  } else {
    hb_buffer_add_utf8(hb_buf, params.text.c_str(), strlen(params.text.c_str()), 0, strlen(params.text.c_str()));
  }
  hb_shape(hb_ft_font, hb_buf, nullptr, 0);

  unsigned int glyph_count;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
  hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);

  glyph_array.reserve(glyph_count);
  for (unsigned int idx = 0; idx < glyph_count; ++idx) {
    FT_Error error;
    FT_UInt glyph_index = glyph_info[idx].codepoint;
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
      LOG(message_group::Warning, params.loc, params.documentPath,
          "Could not load glyph %1$u"
          " for char at index %2$u in text '%3$s'",
          glyph_index, idx, params.text);
      continue;
    }

    FT_Glyph glyph;
    error = FT_Get_Glyph(face->glyph, &glyph);
    if (error) {
      LOG(message_group::Warning, params.loc, params.documentPath,
          "Could not get glyph %1$u"
          " for char at index %2$u in text '%3$s'",
          glyph_index, idx, params.text);
      continue;
    }

    glyph_array.emplace_back(glyph, idx, &glyph_pos[idx]);
  }

  ascent = std::numeric_limits<double>::lowest();
  descent = std::numeric_limits<double>::max();
  advance_x = 0;
  advance_y = 0;
  left = std::numeric_limits<double>::max();
  right = std::numeric_limits<double>::lowest();
  bottom = std::numeric_limits<double>::max();
  top = std::numeric_limits<double>::lowest();

  for (const auto& glyph : glyph_array) {
    FT_BBox bbox;
    FT_Glyph_Get_CBox(glyph.get_glyph(), FT_GLYPH_BBOX_GRIDFIT, &bbox);

    // Note that glyphs can extend left of their origin
    // and right of their advance-width, into the next
    // glyph's box.  In theory they could extend
    // arbitrarily through other glyphs' boxes.  Hence the
    // need for the min/max here.

    // Glyphs with null bounding boxes do not contribute
    // ink and so do not contribute to the bounding box or
    // ascent and descent.
    if (bbox.xMax > bbox.xMin && bbox.yMax > bbox.yMin) {
      ascent = std::max(ascent, bbox.yMax / scale);
      descent = std::min(descent, bbox.yMin / scale);

      const double gxoff = glyph.get_x_offset();
      const double gyoff = glyph.get_y_offset();

      left = std::min(left,
                      advance_x + gxoff + bbox.xMin / scale);
      right = std::max(right,
                       advance_x + gxoff + bbox.xMax / scale);

      top = std::max(top,
                     advance_y + gyoff + bbox.yMax / scale);
      bottom = std::min(bottom,
                        advance_y + gyoff + bbox.yMin / scale);
    }

    advance_x += glyph.get_x_advance() * params.spacing;
    advance_y += glyph.get_y_advance() * params.spacing;
  }

  // Right and left start out reversed.  If any ink is ever
  // contributed they will flip.  If they're still reversed,
  // there was no ink.
  if (right >= left) {
    if (HB_DIRECTION_IS_HORIZONTAL(
          hb_buffer_get_direction(hb_buf))) {
      calc_offsets_horiz(params);
    } else {
      calc_offsets_vert(params);
    }
  } else {
    left = 0;
    right = 0;
    top = 0;
    bottom = 0;
    ascent = 0;
    descent = 0;
    x_offset = 0;
    y_offset = 0;
  }

  ok = true;
}

FreetypeRenderer::ShapeResults::~ShapeResults()
{
  if (hb_buf != nullptr) {
    hb_buffer_destroy(hb_buf);
    hb_buf = nullptr;
  }
  if (hb_ft_font != nullptr) {
    hb_font_destroy(hb_ft_font);
    hb_ft_font = nullptr;
  }
}

FreetypeRenderer::FontMetrics::FontMetrics(
  const FreetypeRenderer::Params& params)
{
  ok = false;

  FT_Face face = params.get_font_face();
  if (face == nullptr) {
    return;
  }

  // scale is the width of an em in 26.6 fractional points
  // @ 100dpi = 100/72 pixels per point
  const FT_Size_Metrics *size_metrics = &face->size->metrics;
  nominal_ascent =
    FT_MulFix(face->ascender, size_metrics->y_scale) / scale
    * params.size;
  nominal_descent =
    FT_MulFix(face->descender, size_metrics->y_scale) / scale
    * params.size;
  max_ascent =
    FT_MulFix(face->bbox.yMax, size_metrics->y_scale) / scale
    * params.size;
  max_descent =
    FT_MulFix(face->bbox.yMin, size_metrics->y_scale) / scale
    * params.size;
  interline =
    FT_MulFix(face->height, size_metrics->y_scale) / scale
    * params.size;
  family_name = face->family_name;
  style_name = face->style_name;

  ok = true;
}

FreetypeRenderer::TextMetrics::TextMetrics(
  const FreetypeRenderer::Params& params)
{
  ok = false;

  ShapeResults sr(params);

  if (!sr.ok) {
    return;
  }

  // Translate all of the metrics from the easy-to-calculate forms
  // to the forms that we want to return, and scale them up to
  // the specified size.

  // Note:  ShapeResults can return {left,right,top,bottom,[xy]_offset}
  // all equal to zero if the text consists only of whitespace.
  // Nothing bad will happen below as a result of these zeroes.
  // We will return a zero-size bounding box at the origin.
  // The advance_[xy] values will be valid and may be non-zero.
  bbox_x = (sr.x_offset + sr.left) * params.size;
  bbox_y = (sr.y_offset + sr.bottom) * params.size;
  bbox_w = (sr.right - sr.left) * params.size;
  bbox_h = (sr.top - sr.bottom) * params.size;

  advance_x = sr.advance_x * params.size;
  advance_y = sr.advance_y * params.size;

  // As with the bounding box, these can be [0,0] if there
  // would be no ink produced.
  // Note: Strictly, I don't know think ascent and descent are needed.
  // I think they are derivable from the bounding box and the
  // offsets.
  ascent = sr.ascent * params.size;
  descent = sr.descent * params.size;

  // The offset values reflect what halign/valign *actually do*
  // to the text.
  x_offset = sr.x_offset * params.size;
  y_offset = sr.y_offset * params.size;

  ok = true;
}

std::vector<std::shared_ptr<const Geometry>> FreetypeRenderer::render(const FreetypeRenderer::Params& params) const
{
  ShapeResults sr(params);

  if (!sr.ok) {
    return {};
  }

  DrawingCallback callback(params.segments, params.size);
  for (const auto& glyph : sr.glyph_array) {
    callback.start_glyph();
    callback.set_glyph_offset(
      sr.x_offset + glyph.get_x_offset(),
      sr.y_offset + glyph.get_y_offset());
    FT_Outline outline = reinterpret_cast<FT_OutlineGlyph>(glyph.get_glyph())->outline;
    FT_Outline_Decompose(&outline, &funcs, &callback);

    double adv_x = glyph.get_x_advance() * params.spacing;
    double adv_y = glyph.get_y_advance() * params.spacing;
    callback.add_glyph_advance(adv_x, adv_y);
    callback.finish_glyph();
  }

  return callback.get_result();
}

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

#include <memory>
#include <string>
#include <vector>
#include <ostream>

#include "core/Parameters.h"
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
    void set_segments(unsigned int segments) {
      this->segments = segments;
    }
    void set_text(const std::string& text) {
      this->text = text;
    }
    void set_font(const std::string& font) {
      this->font = font;
    }
    void set_direction(const std::string& direction) {
      this->direction = direction;
    }
    void set_language(const std::string& language) {
      this->language = language;
    }
    void set_script(const std::string& script) {
      this->script = script;
    }
    void set_halign(const std::string& halign) {
      this->halign = halign;
    }
    void set_valign(const std::string& valign) {
      this->valign = valign;
    }
    void set_loc(const Location& loc) {
      this->loc = loc;
    }
    void set_documentPath(const std::string& path) {
      this->documentPath = path;
    }
    void set(Parameters& parameters);
    [[nodiscard]] FT_Face get_font_face() const;
    void detect_properties();
    friend std::ostream& operator<<(std::ostream& stream, const FreetypeRenderer::Params& params) {
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
    double size, spacing, fn, fa, fs;
    unsigned int segments;
    std::string text, font, direction, language, script, halign, valign;
    Location loc = Location::NONE;
    std::string documentPath = "";
    static bool is_ignored_script(const hb_script_t script);
    hb_script_t detect_script(hb_glyph_info_t *glyph_info,
                              unsigned int glyph_count) const;
    [[nodiscard]] hb_direction_t detect_direction(const hb_script_t script) const;

    friend class FreetypeRenderer;
  };

  class TextMetrics
  {
public:
    bool ok; // true if object is valid
    // The values here are all at their final size; they have been
    // descaled down from the 1e5 size used for Freetype, and rescaled
    // up to the specified size.
    double bbox_x;
    double bbox_y;
    double bbox_w;
    double bbox_h;
    double advance_x;
    double advance_y;
    double ascent;
    double descent;
    double x_offset;
    double y_offset;
    TextMetrics(const FreetypeRenderer::Params& params);
  };
  class FontMetrics
  {
public:
    bool ok; // true if object is valid
    // The values here are all at their final size; they have been
    // descaled down from the 1e5 size used for Freetype, and rescaled
    // up to the specified size.
    double nominal_ascent;
    double nominal_descent;
    double max_ascent;
    double max_descent;
    double interline;
    std::string family_name;
    std::string style_name;
    FontMetrics(const FreetypeRenderer::Params& params);
  };
  FreetypeRenderer();
  virtual ~FreetypeRenderer() = default;

  [[nodiscard]] std::vector<std::shared_ptr<const class Geometry>> render(const FreetypeRenderer::Params& params) const;
private:
  const static double scale;
  FT_Outline_Funcs funcs;

  // The GlyphData assumes responsibility for "glyph" and will ensure
  // that it is freed when the GlyphData is destroyed.
  // However, glyph_pos points to data that the caller must ensure
  // remains valid until the GlyphData is destroyed.
  class GlyphData
  {
public:
    GlyphData(FT_Glyph glyph, unsigned int idx, hb_glyph_position_t *glyph_pos) : glyph(glyph), idx(idx), glyph_pos(glyph_pos) {}
    [[nodiscard]] unsigned int get_idx() const { return idx; }
    [[nodiscard]] FT_Glyph get_glyph() const { return glyph; }
    [[nodiscard]] double get_x_offset() const { return glyph_pos->x_offset / scale; }
    [[nodiscard]] double get_y_offset() const { return glyph_pos->y_offset / scale; }
    [[nodiscard]] double get_x_advance() const { return glyph_pos->x_advance / scale; }
    [[nodiscard]] double get_y_advance() const { return glyph_pos->y_advance / scale; }
    ~GlyphData() { FT_Done_Glyph(glyph); }
private:
    FT_Glyph glyph;
    unsigned int idx;
    hb_glyph_position_t *glyph_pos;
  };

  class ShapeResults
  {
public:
    bool ok{false}; // true if object is valid
    // The values here are all in fractions of the specified size.
    // They have been downscaled from the 1e+5 unit size used for
    // when rendering from Freetype, and have not yet been scaled
    // back up to the desired font size.
    std::vector<GlyphData> glyph_array;
    double x_offset{0.0};
    double y_offset{0.0};
    double left{0.0};
    double right{0.0};
    double top{0.0};
    double bottom{0.0};
    double advance_x{0.0};
    double advance_y{0.0};
    double ascent{0.0};
    double descent{0.0};
    ShapeResults(const FreetypeRenderer::Params& params);
    virtual ~ShapeResults();
private:
    void calc_offsets_horiz(const FreetypeRenderer::Params& params);
    void calc_offsets_vert(const FreetypeRenderer::Params& params);
    hb_font_t *hb_ft_font{nullptr};
    hb_buffer_t *hb_buf{nullptr};
  };

  static int outline_move_to_func(const FT_Vector *to, void *user);
  static int outline_line_to_func(const FT_Vector *to, void *user);
  static int outline_conic_to_func(const FT_Vector *c1, const FT_Vector *to, void *user);
  static int outline_cubic_to_func(const FT_Vector *c1, const FT_Vector *c2, const FT_Vector *to, void *user);
};

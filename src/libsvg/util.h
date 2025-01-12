/*
 * The MIT License
 *
 * Copyright (c) 2016-2018, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <ostream>
#include <string>

namespace libsvg {

// https://oreillymedia.github.io/Using_SVG/guide/units.html
//
// Points (pt)
//
// 1pt ≅ 1.3333px or user units (1px = 0.75pt)
// 1pt = 1/72in
//
// Picas (pc)
//
// 1pc = 16px or user units
// 1pc = 1/6in
//
enum class unit_t { UNDEFINED, NONE, PERCENT, EM, EX, PX, IN, CM, MM, PT, PC };

enum class align_t { UNDEFINED, NONE, MIN, MID, MAX };

struct length_struct {
  double number;
  std::string unit;
};

struct length_t {
  double number;
  unit_t unit;
};

struct viewbox_t {
  double x;
  double y;
  double width;
  double height;
  bool is_valid;
};

struct alignment_t {
  align_t x;
  align_t y;
  bool defer;
  bool meet;
};

double parse_double(const std::string& number);
const length_t parse_length(const std::string& value);
const viewbox_t parse_viewbox(const std::string& value);
const alignment_t parse_alignment(const std::string& value);

std::ostream& operator<<(std::ostream& stream, const unit_t& unit);
std::ostream& operator<<(std::ostream& stream, const length_t& length);
std::ostream& operator<<(std::ostream& stream, const align_t& align);

} // namespace libsvg

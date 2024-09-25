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

#include <map>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <iostream>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <boost/optional.hpp>

#include "polyclipping/clipper.hpp"

namespace libsvg {
class shape;
}

// ccox - I don't like putting this here, but the svg library code did not plan ahead for app customization.
// And this is one of the few sensible places to put it without adding new header files.
struct fnContext {
  fnContext(double fNN, double fSS, double fAA) : fn(fNN), fs(fSS), fa(fAA) {
  }
  bool match(bool val) { if (val) matches++; return val; }
  bool has_matches() { return matches.load() > 0; }

  double fn;
  double fs;
  double fa;
  std::function<bool (const libsvg::shape *)> selector;
private:
  std::atomic<int> matches{0};
};

namespace libsvg {

using path_t = std::vector<Eigen::Vector3d>;
using path_list_t = std::vector<path_t>;
using attr_map_t = std::map<std::string, std::string>;

class shape
{
private:
  shape *parent{nullptr};
  std::vector<shape *> children;

protected:
  boost::optional<std::string> id;
  boost::optional<std::string> layer;
  double x{0};
  double y{0};
  path_list_t path_list;
  std::string transform;
  std::string stroke_width;
  std::string stroke_linecap;
  std::string stroke_linejoin;
  std::string style;
  bool excluded{false};
  bool selected{false};

  [[nodiscard]] double get_stroke_width() const;
  [[nodiscard]] ClipperLib::EndType get_stroke_linecap() const;
  [[nodiscard]] ClipperLib::JoinType get_stroke_linejoin() const;
  [[nodiscard]] const std::string get_style(const std::string& name) const;
  void draw_ellipse(path_t& path, double x, double y, double rx, double ry, void *context);
  void offset_path(path_list_t& path_list, path_t& path, double stroke_width, ClipperLib::EndType stroke_linecap);
  void collect_transform_matrices(std::vector<Eigen::Matrix3d>& matrices, shape *s);

public:
  shape() = default;
  virtual ~shape() = default;

  [[nodiscard]] virtual shape *get_parent() const { return parent; }
  virtual void set_parent(shape *s) { parent = s; }
  virtual void add_child(shape *s) { children.push_back(s); s->set_parent(this); }
  [[nodiscard]] virtual const std::vector<shape *>& get_children() const { return children; }

  [[nodiscard]] virtual bool has_id() const { return id.is_initialized(); }
  [[nodiscard]] virtual const std::string& get_id() const { return id.get(); }
  [[nodiscard]] virtual const std::string get_id_or_default(const std::string& def = "") const { return id.get_value_or(def); }
  [[nodiscard]] virtual bool has_layer() const { return layer.is_initialized(); }
  [[nodiscard]] virtual const std::string& get_layer() const { return layer.get(); }
  [[nodiscard]] virtual double get_x() const { return x; }
  [[nodiscard]] virtual double get_y() const { return y; }

  [[nodiscard]] virtual const path_list_t& get_path_list() const { return path_list; }

  [[nodiscard]] virtual bool is_excluded() const;
  [[nodiscard]] virtual bool is_container() const { return false; }

  virtual void apply_transform();

  [[nodiscard]] virtual const std::string& get_name() const = 0;
  virtual void set_attrs(attr_map_t& attrs, void *context);
  [[nodiscard]] virtual const std::string dump() const { return ""; }

  static shape *create_from_name(const char *name);

  [[nodiscard]] virtual shape *clone() const = 0;
  std::vector<std::shared_ptr<shape>> clone_children();

private:
  friend std::ostream& operator<<(std::ostream& os, const shape& s);
};

} // namespace libsvg

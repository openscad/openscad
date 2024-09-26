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

#include <cstdint>
#include <memory>
#include <cstddef>
#include <string>
#include <vector>

#include "core/node.h"
#include "core/Value.h"

struct img_data_t
{
public:
  using storage_type = double; // float could be enough here

  img_data_t() { min_val = 0; height = width = 0; }

  void clear() { min_val = 0; height = width = 0; storage.clear(); }

  void reserve(size_t x) { storage.reserve(x); }

  void resize(size_t x) { storage.resize(x); }

  storage_type& operator[](int x) { return storage[x]; }

  storage_type min_value() { return min_val; } // *std::min_element(storage.begin(), storage.end());

public:
  unsigned int height; // rows
  unsigned int width; // columns
  storage_type min_val;
  std::vector<storage_type> storage;

};


class SurfaceNode : public LeafNode
{
public:
  VISITABLE();
  SurfaceNode(const ModuleInstantiation *mi) : LeafNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "surface"; }

  Filename filename;
  bool center{false};
  bool invert{false};
  int convexity{1};

  std::unique_ptr<const Geometry> createGeometry() const override;
private:
  void convert_image(img_data_t& data, std::vector<uint8_t>& img, unsigned int width, unsigned int height) const;
  bool is_png(std::vector<uint8_t>& img) const;
  img_data_t read_dat(std::string filename) const;
  img_data_t read_png_or_dat(std::string filename) const;
};

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

#include "core/SurfaceNode.h"

#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "core/Builtins.h"
#include "core/Children.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"
#include "lodepng/lodepng.h"

#include <algorithm>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <filesystem>
namespace fs = std::filesystem;


static std::shared_ptr<AbstractNode> builtin_surface(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  auto node = std::make_shared<SurfaceNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"file", "center", "convexity"}, {"invert"});

  std::string fileval = parameters["file"].isUndefined() ? "" : parameters["file"].toString();
  auto filename = lookup_file(fileval, inst->location().filePath().parent_path().string(), parameters.documentRoot());
  node->filename = filename;
  handle_dep(fs::path(filename).generic_string());

  if (parameters["center"].type() == Value::Type::BOOL) {
    node->center = parameters["center"].toBool();
  }

  if (parameters["convexity"].type() == Value::Type::NUMBER) {
    node->convexity = static_cast<int>(parameters["convexity"].toDouble());
  }

  if (parameters["invert"].type() == Value::Type::BOOL) {
    node->invert = parameters["invert"].toBool();
  }

  return node;
}

void SurfaceNode::convert_image(img_data_t& data, std::vector<uint8_t>& img, unsigned int width, unsigned int height) const
{
  data.width = width;
  data.height = height;
  data.resize( (size_t)width * height);
  double min_val = 200;
  for (unsigned int y = 0; y < height; ++y) {
    for (unsigned int x = 0; x < width; ++x) {
      long idx = 4l * (y * width + x);
      double pixel = 0.2126 * img[idx] + 0.7152 * img[idx + 1] + 0.0722 * img[idx + 2];
      double z = 100.0 / 255 * (invert ? 1 - pixel : pixel);
      data[ x + (width * (height - 1 - y)) ] = z;
      min_val = std::min(z, min_val);
    }
  }
  data.min_val = min_val;
}

bool SurfaceNode::is_png(std::vector<uint8_t>& png) const
{
  const size_t pngHeaderLength = 8;
  const uint8_t pngHeader[pngHeaderLength] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
  return (png.size() >= pngHeaderLength &&
          std::memcmp(png.data(), pngHeader, pngHeaderLength) == 0);
}

img_data_t SurfaceNode::read_png_or_dat(std::string filename) const
{
  img_data_t data;
  std::vector<uint8_t> png;
  int ret_val = 0;
  try{
    ret_val = lodepng::load_file(png, filename);
  } catch (std::bad_alloc& ba) {
    LOG(message_group::Warning, "bad_alloc caught for '%1$s'.", ba.what());
    return data;
  }

  if (ret_val == 78) {
    LOG(message_group::Warning, "The file '%1$s' couldn't be opened.", filename);
    return data;
  }

  if (!is_png(png)) {
    png.clear();
    return read_dat(filename);
  }

  unsigned int width, height;
  std::vector<uint8_t> img;
  auto error = lodepng::decode(img, width, height, png);
  if (error) {
    LOG(message_group::Warning, "Can't read PNG image '%1$s'", filename);
    data.clear();
    return data;
  }

  convert_image(data, img, width, height);

  return data;
}

img_data_t SurfaceNode::read_dat(std::string filename) const
{
  img_data_t data;
  std::ifstream stream(filename.c_str());

  if (!stream.good()) {
    LOG(message_group::Warning, "Can't open DAT file '%1$s'.", filename);
    return data;
  }

  int lines = 0, columns = 0;
  double min_val = 1; // this balances out with the (min_val-1) inside createGeometry, to match old behavior

  using tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> sep(" \t");

  // We use an unordered map because the data file may not be rectangular,
  // and we may need to fill in some bits.
  using unordered_image_data_t = std::unordered_map<std::pair<int, int>, double, boost::hash<std::pair<int, int>>>;
  unordered_image_data_t unordered_data;

  while (!stream.eof()) {
    std::string line;
    while (!stream.eof() && (line.size() == 0 || line[0] == '#')) {
      std::getline(stream, line);
      boost::trim(line);
    }
    if (line.size() == 0 && stream.eof()) break;

    int col = 0;
    tokenizer tokens(line, sep);
    try {
      for (const auto& token : tokens) {
        auto v = boost::lexical_cast<double>(token);
        unordered_data[ std::make_pair(lines, col++) ] = v;
        if (col > columns) columns = col;
        min_val = std::min(v, min_val);
      }
    } catch (const boost::bad_lexical_cast& blc) {
      if (!stream.eof()) {
        LOG(message_group::Warning, "Illegal value in '%1$s': %2$s", filename, blc.what());
      }
      return data;
    }

    lines++;
  }

  data.width = columns;
  data.height = lines;
  data.min_val = min_val;

  // Now convert the unordered, possibly non-rectangular data into a well ordered vector
  // for faster access and reduced memory usage.
  data.resize( (size_t)lines * columns);
  for (int i = 0; i < lines; ++i)
    for (int j = 0; j < columns; ++j)
      data[ i * columns + j ] = unordered_data[std::make_pair(i, j)];

  return data;
}

// FIXME: Look for faster way to generate PolySet directly
std::unique_ptr<const Geometry> SurfaceNode::createGeometry() const
{
  auto data = read_png_or_dat(filename);

  int lines = data.height;
  int columns = data.width;
  double min_val = data.min_value() - 1; // make the bottom solid, and match old code

  // reserve the polygon vector size so we don't have to reallocate as often

  double ox = center ? -(columns - 1) / 2.0 : 0;
  double oy = center ? -(lines - 1) / 2.0 : 0;

  int num_indices = (lines - 1) * (columns - 1) * 4 + (lines - 1) * 2 + (columns - 1) * 2 + 1;
  PolySetBuilder builder(0, num_indices);
  builder.setConvexity(convexity);
  // the bulk of the heightmap
  for (int i = 1; i < lines; ++i)
    for (int j = 1; j < columns; ++j) {
      double v1 = data[ (j - 1) + (i - 1) * columns ];
      double v2 = data[ (j) + (i - 1) * columns ];
      double v3 = data[ (j - 1) + (i) * columns ];
      double v4 = data[ (j) + (i) * columns ];

      double vx = (v1 + v2 + v3 + v4) / 4;

      builder.appendPolygon({
                Vector3d(ox + j - 1, oy + i - 1, v1),
                Vector3d(ox + j, oy + i - 1, v2),
                Vector3d(ox + j - 0.5, oy + i - 0.5, vx)
                });

      builder.appendPolygon({
                Vector3d(ox + j, oy + i - 1, v2),
                Vector3d(ox + j, oy + i, v4),
                Vector3d(ox + j - 0.5, oy + i - 0.5, vx)
                });

      builder.appendPolygon({
                Vector3d(ox + j, oy + i, v4),
                Vector3d(ox + j - 1, oy + i, v3),
                Vector3d(ox + j - 0.5, oy + i - 0.5, vx)
                });

      builder.appendPolygon({
                Vector3d(ox + j - 1, oy + i, v3),
                Vector3d(ox + j - 1, oy + i - 1, v1),
                Vector3d(ox + j - 0.5, oy + i - 0.5, vx)
                });
    }

  // edges along Y
  for (int i = 1; i < lines; ++i) {
    double v1 = data[ (0) + (i - 1) * columns ];
    double v2 = data[ (0) + (i) * columns ];
    double v3 = data[ (columns - 1) + (i - 1) * columns ];
    double v4 = data[ (columns - 1) + (i) * columns ];


    builder.appendPolygon({
        Vector3d(ox + 0, oy + i - 1, min_val),
        Vector3d(ox + 0, oy + i - 1, v1),
        Vector3d(ox + 0, oy + i, v2),
        Vector3d(ox + 0, oy + i, min_val)
    });

    builder.appendPolygon({
        Vector3d(ox + columns - 1, oy + i, min_val),
        Vector3d(ox + columns - 1, oy + i, v4),
        Vector3d(ox + columns - 1, oy + i - 1, v3),
        Vector3d(ox + columns - 1, oy + i - 1, min_val)
    });
  }

  // edges along X
  for (int i = 1; i < columns; ++i) {
    double v1 = data[ (i - 1) + (0) * columns ];
    double v2 = data[ (i) + (0) * columns ];
    double v3 = data[ (i - 1) + (lines - 1) * columns ];
    double v4 = data[ (i) + (lines - 1) * columns ];

    builder.appendPolygon({
        Vector3d(ox + i, oy + 0, min_val),
        Vector3d(ox + i, oy + 0, v2),
        Vector3d(ox + i - 1, oy + 0, v1),
        Vector3d(ox + i - 1, oy + 0, min_val)
    });

    builder.appendPolygon({
        Vector3d(ox + i - 1, oy + lines - 1, min_val),
        Vector3d(ox + i - 1, oy + lines - 1, v3),
        Vector3d(ox + i, oy + lines - 1, v4),
        Vector3d(ox + i, oy + lines - 1, min_val)
    });
  }

  // the bottom of the shape (one less than the real minimum value), making it a solid volume
  if (columns > 1 && lines > 1) {
    builder.beginPolygon(2 * (columns - 1) + 2 * (lines - 1) );
    for (int i = 0; i < lines - 1; ++i)
      builder.addVertex(Vector3d(ox + 0, oy + i, min_val));
    for (int i = 0; i < columns - 1; ++i)
      builder.addVertex(Vector3d(ox + i, oy + lines - 1, min_val));
    for (int i = lines - 1; i > 0; i--)
      builder.addVertex(Vector3d(ox + columns - 1, oy + i, min_val));
    for (int i = columns - 1; i > 0; i--)
      builder.addVertex(Vector3d(ox + i, oy + 0, min_val));
  }

  return builder.build();
}

std::string SurfaceNode::toString() const
{
  std::ostringstream stream;
  fs::path path{static_cast<std::string>(this->filename)}; // gcc-4.6

  stream << this->name() << "(file = " << this->filename
         << ", center = " << (this->center ? "true" : "false")
         << ", invert = " << (this->invert ? "true" : "false")
         << ", " "timestamp = " << fs_timestamp(path)
         << ")";

  return stream.str();
}

void register_builtin_surface()
{
  Builtins::init("surface", new BuiltinModule(builtin_surface),
  {
    "surface(string, center = false, invert = false, number)",
  });
}

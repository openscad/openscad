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

#include "module.h"
#include "ModuleInstantiation.h"
#include "core/node.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "Builtins.h"
#include "Children.h"
#include "Parameters.h"
#include "printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"
#include "ext/lodepng/lodepng.h"
#include "SurfaceNode.h"

#include <cstdint>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


static std::shared_ptr<AbstractNode> builtin_surface(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  auto node = std::make_shared<SurfaceNode>(inst);

  Parameters parameters =
      Parameters::parse(std::move(arguments), inst->location(),
                        {"file", "center", "convexity"}, {"invert", "doubleSided", "thickness", "pixelStep"});

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

  if (parameters["doubleSided"].type() == Value::Type::BOOL) {
    node->doubleSided = parameters["doubleSided"].toBool();
  }

  if (parameters["thickness"].type() == Value::Type::NUMBER) {
    node->thickness = parameters["thickness"].toDouble();
  }

  if (parameters["pixelStep"].type() == Value::Type::NUMBER) {
    node->pixelStep = static_cast<int>(parameters["pixelStep"].toDouble());
    if (node->pixelStep < 1) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "surface(..., pixelStep=%1$s) pixelStep parameter can not be less than 1, reset to 1.", parameters["pixelStep"].toEchoStringNoThrow());
      node->pixelStep = 1;
    }
  }

  return node;
}

void SurfaceNode::convert_image(img_data_t& data, std::vector<uint8_t>& img, unsigned int width, unsigned int height) const
{
  data.width = width;
  data.height = height;
  data.reserve( (size_t)width * height);
  double min_val = 0;
  for (unsigned int y = 0; y < height; ++y) {
    for (unsigned int x = 0; x < width; ++x) {
      long idx = 4l * (y * width + x);
      double pixel = 0.2126 * img[idx] + 0.7152 * img[idx + 1] + 0.0722 * img[idx + 2];
      double z = 100.0 / 255 * (invert ? 255 - pixel : pixel);
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

  const int columns = data.width / pixelStep;
  const int rows = data.height / pixelStep;

  const int width = data.width;
  const int height = data.height;

  const double xStep = static_cast<double>(width-1) / static_cast<double>(columns-1);
  const double yStep = static_cast<double>(height-1) / static_cast<double>(rows-1);

  const double min_val = data.min_value() - thickness; // make the bottom solid, and match old code

  const double ox = center ? -(width - 1) / 2.0 : 0;
  const double oy = center ? -(height - 1) / 2.0 : 0;

  // reserve the polygon vector size so we don't have to reallocate as often
  int numIndices = ((rows - 1) * (columns - 1) * 4); // heightmap (on top)
  numIndices += ((rows - 1) * 2 + (columns - 1) * 2); // sides
  numIndices += doubleSided ? ((rows - 1) * (columns - 1) * 4) : 1; // bottom (heightmap or plane)

  int numVertices = (rows * columns);
  numVertices += doubleSided ? (rows * columns) : ((rows + columns - 4) * 2);
                      
  PolySetBuilder builder(numVertices, numIndices);
  builder.setConvexity(convexity);

  // the bulk of the heightmap
  for (int i = 1; i < rows; ++i) {
      const int topIdx = (i - 1) * pixelStep;
      const int bottomIdx = i * pixelStep;
      const double top = oy + (i - 1) * yStep;
      const double bottom = oy + i * yStep;
      const double yCenter = oy + (i - 0.5) * yStep;

      for (int j = 1; j < columns; ++j) {
        const int leftIdx = (j - 1) * pixelStep;
        const int rightIdx = j * pixelStep;
        const double left = ox + (j - 1) * xStep;
        const double right = ox + j * xStep;
        const double xCenter = ox + (j - 0.5) * xStep;

        const double v1 = data[leftIdx  + topIdx  * data.width];
        const double v2 = data[rightIdx + topIdx  * data.width];
        const double v3 = data[leftIdx  + bottomIdx * data.width];
        const double v4 = data[rightIdx + bottomIdx * data.width];

        const double vx = (v1 + v2 + v3 + v4) / 4.0;

        const Vector3d topLeft    (left,  top,  v1);
        const Vector3d topRight   (right, top,  v2);
        const Vector3d bottomLeft (left,  bottom, v3);
        const Vector3d bottomRight(right, bottom, v4);
        const Vector3d center     (xCenter, yCenter, vx);

        builder.appendPoly({topLeft, topRight, center});
        builder.appendPoly({bottomLeft, topLeft, center});
        builder.appendPoly({bottomRight, bottomLeft, center});
        builder.appendPoly({topRight, bottomRight, center});
    }
  }

  if (doubleSided) {
    for (int i = 1; i < rows; ++i) {
      const int topIdx = (i - 1) * pixelStep;
      const int bottomIdx = i * pixelStep;
      const double top = oy + (i - 1) * yStep;
      const double bottom = oy + i * yStep;
      const double yCenter = oy + (i - 0.5) * yStep;

      for (int j = 1; j < columns; ++j) {
        const int leftIdx = (j - 1) * pixelStep;
        const int rightIdx = j * pixelStep;
        const double left = ox + (j - 1) * xStep;
        const double right = ox + j * xStep;
        const double xCenter = ox + (j - 0.5) * xStep;

        const double v1 = data[leftIdx  + topIdx  * data.width] - thickness;
        const double v2 = data[rightIdx + topIdx  * data.width] - thickness;
        const double v3 = data[leftIdx  + bottomIdx * data.width] - thickness;
        const double v4 = data[rightIdx + bottomIdx * data.width] - thickness;

        const double vx = (v1 + v2 + v3 + v4) / 4.0;

        const Vector3d topLeft    (left,  top,  v1);
        const Vector3d topRight   (right, top,  v2);
        const Vector3d bottomLeft (left,  bottom, v3);
        const Vector3d bottomRight(right, bottom, v4);
        const Vector3d center     (xCenter, yCenter, vx);

        builder.appendPoly({topLeft, topRight, center});
        builder.appendPoly({bottomLeft, topLeft, center});
        builder.appendPoly({bottomRight, bottomLeft, center});
        builder.appendPoly({topRight, bottomRight, center});
      }
    }
  }
  else if (columns > 1 && rows > 1) {
    // the bottom of the shape (one less than the real minimum value), making it a
    // solid volume
    builder.appendPoly(2 * (columns - 1) + 2 * (rows - 1));
    for (int i = 0; i < columns - 1; ++i)
      builder.prependVertex(
          builder.vertexIndex(Vector3d(ox + i * xStep, oy + 0, min_val)));
    for (int i = 0; i < rows - 1; ++i)
      builder.prependVertex(
          builder.vertexIndex(Vector3d(ox + width - 1, oy + i * yStep, min_val)));
    for (int i = columns - 1; i > 0; --i)
      builder.prependVertex(
          builder.vertexIndex(Vector3d(ox + i * xStep, oy + height - 1, min_val)));
    for (int i = rows - 1; i > 0; --i)
      builder.prependVertex(
          builder.vertexIndex(Vector3d(ox + 0, oy + i * yStep, min_val)));
  }

  // edges along Y
  for (int i = 1; i < rows; ++i) {
    const int topIdx = (i - 1) * pixelStep;
    const int bottomIdx = i * pixelStep;
    const double top = oy + (i - 1) * yStep;
    const double bottom = oy + i * yStep;

    const double v1 = data[(0)           + topIdx    * data.width];
    const double v2 = data[(0)           + bottomIdx * data.width];
    const double v3 = data[(columns - 1) * pixelStep + topIdx  * data.width];
    const double v4 = data[(columns - 1) * pixelStep + bottomIdx * data.width];

    builder.appendPoly({
      Vector3d(ox + 0, top, doubleSided ? v1 - thickness : min_val),
      Vector3d(ox + 0, top, v1),
      Vector3d(ox + 0, bottom,v2),
      Vector3d(ox + 0, bottom,doubleSided ? v2 - thickness : min_val)
    });

    builder.appendPoly({
      Vector3d(ox + width - 1, bottom,doubleSided ? v4 - thickness : min_val),
      Vector3d(ox + width - 1, bottom,v4),
      Vector3d(ox + width - 1, top, v3),
      Vector3d(ox + width - 1, top, doubleSided ? v3 - thickness : min_val)
    });
  }

  // edges along X
  for (int i = 1; i < columns; ++i) {
    const int leftIdx = (i - 1) * pixelStep;
    const int rightIdx = i * pixelStep;
    const double left = ox + (i - 1) * xStep;
    const double right = ox + i * xStep;

    const double v1 = data[leftIdx  + (0) * data.width];
    const double v2 = data[rightIdx + (0) * data.width];
    const double v3 = data[leftIdx  + (rows - 1) * pixelStep * data.width];
    const double v4 = data[rightIdx + (rows - 1) * pixelStep * data.width];

    builder.appendPoly({
      Vector3d(right, oy + 0, doubleSided ? v2 - thickness : min_val),
      Vector3d(right, oy + 0, v2),
      Vector3d(left,  oy + 0, v1),
      Vector3d(left,  oy + 0, doubleSided ? v1 - thickness : min_val)
    });

    builder.appendPoly({
      Vector3d(left,  oy + height - 1, doubleSided ? v3 - thickness : min_val),
      Vector3d(left,  oy + height - 1, v3),
      Vector3d(right, oy + height - 1, v4),
      Vector3d(right, oy + height - 1, doubleSided ? v4 - thickness : min_val)
    });
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
         << ", doubleSided = " << (this->doubleSided ? "true" : "false")
         << ", thickness = " << this->thickness
         << ", pixelStep = " << this->pixelStep
         << ", "
            "timestamp = "
         << (fs::exists(path) ? fs::last_write_time(path) : 0) << ")";

  return stream.str();
}

void register_builtin_surface()
{
  Builtins::init("surface", new BuiltinModule(builtin_surface),
                 {
                     "surface(string, center = false, invert = false, convexity = number, doubleSided = false, thickness = 1, pixelStep = 1)",
                 });
}

/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *  Copyright (C) 2021      Konstantin Podsvirov <konstantin@podsvirov.pro>
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

#include "io/export.h"
#include "geometry/linalg.h"
#include "Feature.h"
#include "geometry/Reindexer.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"

#include <ostream>
#include <memory>
#include <cstddef>
#include <cstdint>

uint8_t clamp_color_channel(float value)
{
  if (value < 0) return 0;
  if (value > 1) return 255;
  return (uint8_t)(value * 255);
}

void export_off(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  auto ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
    ps = createSortedPolySet(*ps);
  }
  const auto& v = ps->vertices;
  size_t numverts = v.size();


  output << "OFF " << numverts << " " << ps->indices.size() << " 0\n";
  for (size_t i = 0; i < numverts; ++i) {
    output << v[i][0] << " " << v[i][1] << " " << v[i][2] << " " << "\n";
  }

  auto has_color = !ps->color_indices.empty();
  
  for (size_t i = 0; i < ps->indices.size(); ++i) {
    int nverts = ps->indices[i].size();
    output << nverts;
    for (size_t n = 0; n < nverts; ++n) output << " " << ps->indices[i][n];
    if (has_color) {
      auto color_index = ps->color_indices[i];
      if (color_index >= 0) {
        auto color = ps->colors[color_index];
        auto r = clamp_color_channel(color[0]);
        auto g = clamp_color_channel(color[1]);
        auto b = clamp_color_channel(color[2]);
        auto a = clamp_color_channel(color[3]);
        output << " " << (int)r << " " << (int)g << " " << (int)b;
        // Alpha channel is read by apps like MeshLab.
        if (a != 255) output << " " << (int)a;
      }
    }
    output << "\n";
  }
}

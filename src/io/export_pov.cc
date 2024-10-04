/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2021      Konstantin Podsvirov <konstantin@podsvirov.pro>
 *         Povray 2024      Folkert van Heusden  <mail@vanheusden.com>
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

#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"


void export_pov(const std::shared_ptr<const Geometry>& geom, std::ostream& output, const ExportInfo& exportInfo)
{
  auto ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
    ps = createSortedPolySet(*ps);
  }

  output << "// Generated by OpenSCAD!\n";
  output << "// Source file: " << exportInfo.sourceFileName << "\n\n";

  output << "#version 3.7;\n";
  output << "global_settings { assumed_gamma 1.0 }\n";
  output << "#declare MATERIAL=finish { specular 0.5 roughness 0.001 reflection{0 0.63 fresnel} ambient 0 diffuse 0.6 conserve_energy }\n";
  output << "#declare MATERIAL_INT=interior{ior 1.32}\n";

  auto has_color = !ps->color_indices.empty();

  for (size_t polygon_index=0; polygon_index<ps->indices.size(); polygon_index++) {
    const auto &polygon = ps->indices[polygon_index];
    output << "polygon { " << polygon.size() + 1 << ", \n";
    for (size_t i=0; i<polygon.size(); i++) {
      if (i)
        output << ", ";
      const auto & x = ps->vertices[polygon[i]].x();
      const auto & y = ps->vertices[polygon[i]].y();
      const auto & z = ps->vertices[polygon[i]].z();
      output << "<" << x << ", " << y << ", " << z << ">";
    }
    output << ", <" << ps->vertices[polygon[0]].x() << ", " << ps->vertices[polygon[0]].y() << ", " << ps->vertices[polygon[0]].z() << ">";
    float r = 0xf9 / 255., g = 0xd7 / 255., b = 0x2c / 255., f = 0.;  // CGAL_FACE_FRONT_COLOR
    if (has_color) {
      auto color_index = ps->color_indices[polygon_index];
      if (color_index >= 0) {
        auto color = ps->colors[color_index];
        r = color[0];
        g = color[1];
        b = color[2];
        f = 1.0 - color[3];
      }
    }
    if (r == 0 && g == 0 && b == 0)  // work around for black objects
      g = 1.0;
    output << "\n";
    output << "texture { pigment { color rgbf <" << r << ", " << g << ", " << b << ", " << f << "> } }\n";
    output << "finish { MATERIAL } interior { MATERIAL_INT }\n";
    output << "}\n";
  }

  BoundingBox bbox = geom->getBoundingBox();

  auto & min_x = bbox.min().x();
  auto & min_y = bbox.min().y();
  auto & min_z = bbox.min().z();

  auto & max_x = bbox.max().x();
  auto & max_y = bbox.max().y();
  auto & max_z = bbox.max().z();

  double dx = max_x - min_x;
  double dy = max_y - min_y;
  double dz = max_z - min_z;

  constexpr double move_away_factor = 2.;
  std::vector<double> lx { min_x - dx * move_away_factor, bbox.center().x(), max_x + dx * move_away_factor };
  std::vector<double> ly { min_y - dy * move_away_factor, bbox.center().y(), max_y + dy * move_away_factor };
  std::vector<double> lz { min_z - dz * move_away_factor, bbox.center().z(), max_z + dz * move_away_factor };

  constexpr float brightness = 0.2;  // 1.0 is way too bright

  for(auto cur_lx: lx) {
    for(auto cur_ly: ly) {
      for(auto cur_lz: lz)
        output << "light_source { <" << cur_lx << ", " << cur_ly << ", " << cur_lz << "> color rgb <" << brightness << ", " << brightness << ", " << brightness << "> }\n";
    }
  }

  output << "camera { look_at <" << bbox.center().x() << ", " << bbox.center().y() << ", " << bbox.center().z() << "> "
    "location <" << min_x + dx * move_away_factor << ", " << min_y - dy * move_away_factor << ", " << min_z + dz * move_away_factor << "> "
    "up <0, 0, 1> right <1, 0, 0> sky <0, 0, 1> rotate <-55, clock * 3, clock + 25> right x*image_width/image_height }\n";
  output << "#include \"rad_def.inc\"\n";
  output << "global_settings { photons { count 20000 autostop 0 jitter .4 } radiosity { Rad_Settings(Radiosity_Normal, off, off) } }\n";
}

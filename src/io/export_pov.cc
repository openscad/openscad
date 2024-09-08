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

#include "export.h"

#include "PolySet.h"
#include "PolySetUtils.h"

#include <stdlib.h>
void export_pov(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  // FIXME: In lazy union mode, should we export multiple IndexedFaceSets?
  auto ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (Feature::ExperimentalPredictibleOutput.is_enabled()) {
    ps = createSortedPolySet(*ps);
  }

  output << "// hello povray!\n\n";

  double avg_x = 0;
  double avg_y = 0;
  double avg_z = 0;
  double tsd_x = 0;
  double tsd_y = 0;
  double tsd_z = 0;
  size_t avg_n = 0;

  for (const auto &t : ps->indices) {
    output << "polygon { " << t.size() + 1 << ", \n";
    for (size_t i=0; i<t.size(); i++) {
      if (i)
        output << ", ";
      output << "<" << ps->vertices[t.at(i)].x() << ", " << ps->vertices[t.at(i)].y() << ", " << ps->vertices[t.at(i)].z() << ">";
      avg_x += ps->vertices[t.at(i)].x();
      avg_y += ps->vertices[t.at(i)].y();
      avg_z += ps->vertices[t.at(i)].z();
      tsd_x += ps->vertices[t.at(i)].x() * ps->vertices[t.at(i)].x();
      tsd_y += ps->vertices[t.at(i)].y() * ps->vertices[t.at(i)].y();
      tsd_z += ps->vertices[t.at(i)].z() * ps->vertices[t.at(i)].z();
      avg_n++;
    }
    output << ", <" << ps->vertices[t.at(0)].x() << ", " << ps->vertices[t.at(0)].y() << ", " << ps->vertices[t.at(0)].z() << ">";
    output << "\n";
    output << "texture { pigment { color rgb <" << 1.0 << ", " << 1.0 << ", " << 1.0 << "> } }\n";
    output << "}\n";
  }

  double center_x = avg_x / avg_n;
  double center_y = avg_y / avg_n;
  double center_z = avg_z / avg_n;

  double sd_x = sqrt(tsd_x / avg_n - pow(center_x, 2.));
  double sd_y = sqrt(tsd_y / avg_n - pow(center_y, 2.));
  double sd_z = sqrt(tsd_z / avg_n - pow(center_z, 2.));
  double dist = pow(sd_x * sd_x + sd_y * sd_y + sd_z * sd_z, 1/3.);

  double l_x = center_x + dist * 5;
  double l_y = center_y + dist * 5;
  double l_z = center_z + dist * 5;

  output << "light_source { <" <<  l_x << ", " <<  l_y << ", " <<  l_z << "> color rgb <1, 1, 1> }\n";
  output << "light_source { <" << -l_x << ", " <<  l_y << ", " <<  l_z << "> color rgb <1, 1, 1> }\n";
  output << "light_source { <" <<  l_x << ", " << -l_y << ", " <<  l_z << "> color rgb <1, 1, 1> }\n";
  output << "light_source { <" <<  l_x << ", " <<  l_y << ", " << -l_z << "> color rgb <1, 1, 1> }\n";

  output << "camera { look_at <" << center_x << ", " << center_y << ", " << center_z << "> location <" << center_x + dist * 10 << ", " << center_y + dist * 10 << ", " << center_z + dist * 10 << "> }\n";
}

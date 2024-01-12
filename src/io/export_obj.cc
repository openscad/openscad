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

#include "export.h"

#include "PolySetBuilder.h"
#include "PolySet.h"

void export_obj(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  PolySetBuilder builder;
  builder.appendGeometry(geom);
  auto ps = builder.build();

  output << "# OpenSCAD obj exporter\n";

  for (size_t i = 0; i < ps->vertices.size(); ++i) {
    output << "v " << ps->vertices[i][0] << " " << ps->vertices[i][1] << " " << ps->vertices[i][2] << "\n";
  }

  for (int i = 0; i < ps->indices.size(); i++) {

    output << "f ";

    for(int j=0;j<ps->indices[i].size();j++) {
      auto index = ps->indices[i][j];
      output << " " << (1 + index);
    }
    output << "\n";
  }

}

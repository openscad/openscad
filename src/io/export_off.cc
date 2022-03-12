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

#ifdef ENABLE_CGAL

#include "IndexedMesh.h"

void export_off(const shared_ptr<const Geometry>& geom, std::ostream& output)
{
  IndexedMesh mesh;
  mesh.append_geometry(geom);

  output << "OFF " << mesh.vertices.size() << " " << mesh.numfaces << " 0\n";
  const auto& v = mesh.vertices.getArray();
  size_t numverts = mesh.vertices.size();
  for (size_t i = 0; i < numverts; ++i) {
    output << v[i][0] << " " << v[i][1] << " " << v[i][2] << " " << "\n";
  }
  size_t cnt = 0;
  for (size_t i = 0; i < mesh.numfaces; ++i) {
    size_t nverts = 0;
    while (mesh.indices[cnt++] != -1) nverts++;
    output << nverts;
    cnt -= nverts + 1;
    for (size_t n = 0; n < nverts; ++n) output << " " << mesh.indices[cnt++];
    output << "\n";
    cnt++; // Skip the -1 marker
  }

}

#endif // ENABLE_CGAL

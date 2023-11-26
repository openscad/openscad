/*
 *  OpenSCAD (www.openscad.org)
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

#include "IndexedMesh.h"

void export_wrl(const shared_ptr<const Geometry>& geom, std::ostream& output)
{
  IndexedMesh mesh;
  mesh.append_geometry(geom);

  output << "#VRML V2.0 utf8\n\n";

  output << "Shape {\n\n";

  output << "appearance Appearance { material Material {\n";
  output << "ambientIntensity 0.3\n";
  output << "diffuseColor 0.97647 0.843137 0.172549\n";
  output << "specularColor 0.2 0.2 0.2\n";
  output << "shininess 0.3\n";
  output << "} }\n\n";

  output << "geometry IndexedFaceSet {\n\n";

  output << "creaseAngle 0.5\n\n";

  output << "coord Coordinate { point [\n";
  const auto& v = mesh.vertices.getArray();
  const size_t numverts = mesh.vertices.size();
  for (size_t i = 0; i < numverts; ++i) {
    output << v[i][0] << " " << v[i][1] << " " << v[i][2];
    if (i < numverts - 1) {
      output << ",";
    }
    output << "\n";
  }
  output << "] }\n\n";

  output << "coordIndex [\n";
  const size_t numindices = mesh.indices.size();
  for (size_t i = 0; i < numindices; ++i) {
    output << mesh.indices[i];
    if (i < numindices - 1) {
      output << ",";
    }
    if (mesh.indices[i] == -1) {
      output << "\n";
    }
  }
  output << "]\n\n";

  output << "}\n\n";

  output << "}\n";
}

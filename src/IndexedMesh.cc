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

#include "IndexedMesh.h"

#include "export.h"
#include "polyset-utils.h"
#include "dxfdata.h"

#ifdef ENABLE_CGAL

#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"

#include "grid.h"

IndexedMesh::IndexedMesh()
  : numfaces(0)
{
}

void IndexedMesh::append_geometry(const PolySet& ps)
{
  IndexedMesh& mesh = *this;
  for (const auto& p : ps.polygons) {
    for (const auto& v : p) {
      mesh.indices.push_back(mesh.vertices.lookup(v));
    }
    mesh.numfaces++;
    mesh.indices.push_back(-1);
  }
}

void IndexedMesh::append_geometry(const shared_ptr<const Geometry>& geom)
{
  IndexedMesh& mesh = *this;
  if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      mesh.append_geometry(item.second);
    }
  } else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    PolySet ps(3);
    bool err = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3), ps);
    if (err) {
      LOG(message_group::Error, Location::NONE, "", "Nef->PolySet failed");
    } else {
      mesh.append_geometry(ps);
    }
  } else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
    mesh.append_geometry(*ps);
  } else if (dynamic_pointer_cast<const Polygon2d>(geom)) {
    assert(false && "Unsupported file format");
  } else {
    assert(false && "Not implemented");
  }
}

#endif // ENABLE_CGAL

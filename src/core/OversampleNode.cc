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

#include "OversampleNode.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "Children.h"
#include "Parameters.h"
#include "src/utils/printutils.h"
#include "io/fileutils.h"
#include "Builtins.h"
#include "handle_dep.h"
#include "src/geometry/PolySetBuilder.h"

#include <cmath>
#include <sstream>

#include <src/geometry/PolySetUtils.h>
#include <src/core/Tree.h>
#include <src/geometry/GeometryEvaluator.h>
#include <boost/functional/hash.hpp>
#include <src/utils/hash.h>
typedef std::vector<int> intList;

double roundCoord(double c)
{
  if (c > 0) return (int)(c * 1000 + 0.5) / 1000.0;
  else return (int)(c * 1000 - 0.5) / 1000.0;
}

void ov_add_poly(PolySetBuilder& builder, Vector3d p)
{
  builder.addVertex(builder.vertexIndex(p));
}

std::unique_ptr<const Geometry> ov_dynamic(const std::shared_ptr<const PolySet>& ps)
{
  return std::make_unique<PolySet>(*ps);
}

std::unique_ptr<const Geometry> ov_static(const std::shared_ptr<const PolySet>& ps, int n)
{
  // tesselate object
  auto ps_tess = PolySetUtils::tessellate_faces(*ps);
  std::vector<Vector3d> pt_dir;
  std::unordered_map<Vector3d, int, boost::hash<Vector3d> > pointIntMap;
  PolySetBuilder builder_ov(0, 0, 3, true);
  for (size_t i = 0; i < ps_tess->indices.size(); i++) {
    auto& pol = ps_tess->indices[i];
    Vector3d p1 = ps_tess->vertices[pol[0]];
    Vector3d p2 = ps_tess->vertices[pol[1]];
    Vector3d p3 = ps_tess->vertices[pol[2]];
    Vector3d p21 = (p2 - p1) / n;
    Vector3d p31 = (p3 - p1) / n;
    Vector3d botlast, botcur, toplast, topcur;
    double r = 1.0;
    Vector3d center(0, 0, 0);
    for (int j = 0; j < n; j++) {
      botcur = p1 + p31 * j;
      topcur = p1 + p31 * (j + 1);

      for (int k = 0; k < n - j; k++) {
        if (k != 0) {
          toplast = topcur;
          topcur = topcur + p21;
          builder_ov.beginPolygon(3);
          ov_add_poly(builder_ov, botcur);
          ov_add_poly(builder_ov, topcur);
          ov_add_poly(builder_ov, toplast);
        }
        botlast = botcur;
        botcur = botlast + p21;
        builder_ov.beginPolygon(3);
        ov_add_poly(builder_ov, botlast);
        ov_add_poly(builder_ov, botcur);
        ov_add_poly(builder_ov, topcur);
      }
    }
  }
  return builder_ov.build();
}

std::unique_ptr<const Geometry> OversampleNode::createGeometry() const
{
  if (this->children.size() == 0) {
    return std::unique_ptr<PolySet>();
  }
  std::shared_ptr<AbstractNode> child = this->children[0];
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps = PolySetUtils::getGeometryAsPolySet(geom);
  if (ps == nullptr) return std::unique_ptr<PolySet>();
  if (this->method == "dynamic") return ov_dynamic(ps);
  return ov_static(ps, this->n);
}

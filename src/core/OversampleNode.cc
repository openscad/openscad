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

void ov_add_poly_round(PolySetBuilder& builder,
                       std::unordered_map<Vector3d, Vector3d, boost::hash<Vector3d> >& weldMap,
                       Vector3d p, const Vector3d& center, double r, int round, int orgpt)
{
  if (round && !orgpt) {
    p[0] = roundCoord(p[0]);
    p[1] = roundCoord(p[1]);
    p[2] = roundCoord(p[2]);

    Vector3d diff = p - center;
    diff.normalize();
    Vector3d pnew = center + diff * r;
    if (weldMap.count(p) == 0) {
      weldMap[p] = pnew;
    } else {
      weldMap[p] = (weldMap[p] + pnew) / 2.0;
    }
  }
  builder.addVertex(builder.vertexIndex(Vector3d(p[0], p[1], p[2])));
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
  std::shared_ptr<const PolySet> ps = std::dynamic_pointer_cast<const PolySet>(geom);
  // tesselate object
  auto ps_tess = PolySetUtils::tessellate_faces(*ps);
  std::vector<Vector3d> pt_dir;
  std::unordered_map<Vector3d, int, boost::hash<Vector3d> > pointIntMap;
  std::unordered_map<Vector3d, Vector3d, boost::hash<Vector3d> > weldMap;
  if (this->round == 1) {
    // create indexed point list
    std::vector<Vector3d> pointList;       // list of all the points in the object
    std::vector<Vector3d> pointListNew;    // list of all the points in the object
    std::vector<intList> polygons;         // list polygons represented by indexes
    std::vector<intList> pointToFaceInds;  //  mapping pt_ind -> list of polygon inds which use it
    std::vector<intList> pointToFacePoss;  //  mapping pt_ind -> list of polygon inds which use it
    intList emptyList;
    for (size_t i = 0; i < ps_tess->indices.size(); i++) {
      auto pol = ps_tess->indices[i];
      intList polygon;
      for (size_t j = 0; j < pol.size(); j++) {
        int ptind = 0;
        Vector3d pt = ps_tess->vertices[pol[j]];
        pt[0] = roundCoord(pt[0]);
        pt[1] = roundCoord(pt[1]);
        pt[2] = roundCoord(pt[2]);
        if (!pointIntMap.count(pt)) {
          pointList.push_back(pt);
          pointToFaceInds.push_back(emptyList);
          pointToFacePoss.push_back(emptyList);
          ptind = pointList.size() - 1;
          pointIntMap[pt] = ptind;
        } else ptind = pointIntMap[pt];
        polygon.push_back(ptind);
        pointToFaceInds[ptind].push_back(i);
        pointToFacePoss[ptind].push_back(j);
      }
      polygons.push_back(polygon);
    }
    // for each vertex, calculate the dir
    for (size_t i = 0; i < pointList.size(); i++) {
      Vector3d dir(0, 0, 0);
      for (size_t j = 0; j < pointToFaceInds[i].size(); j++) {
        int polind = pointToFaceInds[i][j];
        int polptind = pointToFacePoss[i][j];
        int n = polygons[polind].size();
        Vector3d p0 = pointList[polygons[polind][(polptind + n - 1) % n]];
        Vector3d p1 = pointList[polygons[polind][polptind]];
        Vector3d p2 = pointList[polygons[polind][(polptind + 1) % n]];
        Vector3d d1 = p1 - p0;
        Vector3d d2 = p1 - p2;
        Vector3d norm = d2.cross(d1).normalized();
        double ang = acos(d1.dot(d2) / (d1.norm() * d2.norm()));
        dir = dir + norm * ang;
      }
      dir.normalize();
      pt_dir.push_back(dir);
    }
  }
  PolySetBuilder builder_ov(0, 0, 3, true);
  for (size_t i = 0; i < ps_tess->indices.size(); i++) {
    auto& pol = ps_tess->indices[i];
    Vector3d p1 = ps_tess->vertices[pol[0]];
    Vector3d p2 = ps_tess->vertices[pol[1]];
    Vector3d p3 = ps_tess->vertices[pol[2]];
    Vector3d p21 = (p2 - p1) / this->n;
    Vector3d p31 = (p3 - p1) / this->n;
    Vector3d botlast, botcur, toplast, topcur;
    double r = 1.0;
    Vector3d center(0, 0, 0);
    int round1 = this->round;
    if (round1) {
      Vector3d cutmean(0, 0, 0);
      int results = 0;
      for (int j = 0; j < 3; j++) {  // for all 3 edges
        Vector3d dir1 = ps_tess->vertices[pol[(j + 2) % 3]] - ps_tess->vertices[pol[(j + 1) % 3]];
        Vector3d dir2 = pt_dir[pointIntMap[ps_tess->vertices[pol[(j + 1) % 3]]]];
        Vector3d cut;
        if (!cut_face_line(ps_tess->vertices[pol[(j + 1) % 3]], dir1.cross(dir2),
                           ps_tess->vertices[pol[j]], pt_dir[pointIntMap[ps_tess->vertices[pol[j]]]],
                           cut, NULL)) {
          cutmean = cutmean + cut;
          results++;
        }
      }
      if (results > 0) {
        center = cutmean * (1.0 / results);
        r = (center - p1).norm();  // TODO hier faktor overround
      } else round1 = 0;
      //
    }
    for (int j = 0; j < this->n; j++) {
      botcur = p1 + p31 * j;
      topcur = p1 + p31 * (j + 1);

      for (int k = 0; k < this->n - j; k++) {
        if (k != 0) {
          toplast = topcur;
          topcur = topcur + p21;
          builder_ov.beginPolygon(3);
          ov_add_poly_round(builder_ov, weldMap, botcur, center, r, round1, 0);
          ov_add_poly_round(builder_ov, weldMap, topcur, center, r, round1, 0);
          ov_add_poly_round(builder_ov, weldMap, toplast, center, r, round1, 0);
        }
        botlast = botcur;
        botcur = botlast + p21;
        builder_ov.beginPolygon(3);
        ov_add_poly_round(builder_ov, weldMap, botlast, center, r, round1, j == 0 && k == 0);
        ov_add_poly_round(builder_ov, weldMap, botcur, center, r, round1, j == 0 && k == this->n - 1);
        ov_add_poly_round(builder_ov, weldMap, topcur, center, r, round1, j == this->n - 1);
      }
    }
  }
  auto ps_ov = builder_ov.build();
  for (size_t i = 0; i < ps_ov->indices.size(); i++) {
    for (size_t j = 0; j < ps_ov->indices[i].size(); j++) {
      Vector3d pt = ps_ov->vertices[ps_ov->indices[i][j]];
      if (weldMap.count(pt) > 0) {
        ps_ov->vertices[ps_ov->indices[i][j]] = weldMap[pt];
      }
    }
  }
  return ps_ov;
}

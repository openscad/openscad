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

#include "SurfaceNode.h"
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
#include "lodepng/lodepng.h"

const char *projectionNames[] = {"none",        "triplanar", "cubic",   "spherical",
                                 "cylindrical", "planarx",   "planary", "planarz"};

double BaseProjection::tcoord(double x, double y) const
{
  if (texture.width == 0) return 0;
  double u = x / width;
  double v = y / height;
  // get texture coorindate
  u = u - floor(u);
  v = v - floor(v);
  int tx = (texture.width - 1) * u;
  int ty = (texture.height - 1) * v;
  Vector3f pixel = texture[ty * texture.width + tx];
  double dep = (pixel[0] + pixel[1] + pixel[2]) * depth / (3.0 * 256.0);
  return dep;
}

Vector3d BaseProjection::calcMidpoint(const Vector3d& p1, const Vector3d& p2)
{
  /*
    std::vector<Vector2d> line_start;
    std::vector<Vector2d> line_end;

    // convert p1 and p2 to Uv space
    do {
      if (convertToUv(p1, u1, v1)) break;
      if (convertToUv(p2, u2, v2)) break;
      Vector2d s1(u1, v1);
      Vector2d s2(u2, v2);
      // will project lines into range of uv
      printf("s1 %g/%g s2 %g/%g\n", s1[0], s1[1], s2[0], s2[1]);
      Vector2d s21 = s2 - s1;
      double minu = std::floor(std::min(u1, u2) / width);
      double minv = std::floor(std::min(v1, v2) / height);
      for (int i = 0; i < line_start.size(); i++) {
        Vector2d l1 = Vector2d((line_start[i][0] + minu) * width, (line_start[i][1] + minv) * height);
        Vector2d l2 = Vector2d((line_end[i][0] + minu) * width, (line_end[i][1] + minv) * height);
        printf("l1 %g/%g l2 %g/%g\n", l1[0], l1[1], l2[0], l2[1]);

        Vector2d l21 = l2 - l1;

        double det = s21[0] * l21[1] - s21[1] * l21[0];
        Vector2d tmp = l1 - s1;
        double q_pxs = tmp[0] * s21[1] - tmp[1] * s21[0];

        if (det != 0) {
          tmp = l1 - s1;
          double t = (tmp[0] * l21[1] - tmp[1] * l21[0]) / det;
          double u = (tmp[0] * s21[1] - tmp[1] * s21[0]) / det;

          if (t >= 0.05 && t <= 0.95 && u >= 0 && u <= 1) {
            return calcMidpoint(p1, p2, t);
          }
        }
      }

    } while (0);
  */
  Vector3d mid = (p1 + p2) / 2.0;
  return mid;
}

int TriPlanarProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  return 1;  // Uv conversion is not possible in Triplanar projection
}

Vector3d TriPlanarProjection::calcMidpoint(const Vector3d& a, const Vector3d& b, double x)
{
  return Vector3d(0, 0, 0);
}

Vector3d TriPlanarProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  Vector3d vx(1, 0, 0);
  Vector3d vy(0, 1, 0);
  Vector3d vz(0, 0, 1);

  // triplanar texturing
  return vx * tcoord(pt[1], pt[2]) * n[0] + vy * tcoord(pt[0], pt[2]) * n[1] +
         vz * tcoord(pt[0], pt[1]) * n[2];
}

// ----------------------

int CubicProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  double x = fabs(pt[0]);
  double y = fabs(pt[1]);
  double z = fabs(pt[2]);
  if (x > y && x > z) {
    u = pt[1];
    v = pt[2];
  } else if (y > z) {
    u = pt[0];
    v = pt[2];
  } else {
    u = pt[0];
    v = pt[1];
  }
  return 0;
}
Vector3d CubicProjection::calcMidpoint(const Vector3d& a, const Vector3d& b, double x)
{
  return Vector3d(0, 0, 0);
}

Vector3d CubicProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  double x = fabs(pt[0]);
  double y = fabs(pt[1]);
  double z = fabs(pt[2]);
  if (x > y && x > z) {
    return Vector3d(1, 0, 0) * tcoord(pt[1], pt[2]);
  } else if (y > z) {
    return Vector3d(0, 1, 0) * tcoord(pt[0], pt[2]);
  } else {
    return Vector3d(0, 0, 1) * tcoord(pt[0], pt[1]);
  }
}

// ----------------------

int SphericalProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  Vector3d ptc = pt - center;
  u = 10 * (atan2(ptc[1], ptc[0]) / (2 * 3.1415926) + 0.5);
  v = 10 * acos(pt[2] / pt.norm()) / 3.1415926;
  return 0;
}

Vector3d SphericalProjection::calcMidpoint(const Vector3d& p1, const Vector3d& p2, double x)
{
  return p1 + (p2 - p1) * x;
  /*
  printf("Calcmidpopint\n");
double u1, v1, r1, u2, v2, r2;
convertToUv(p1, u1, v1);
r1 = p1.norm();

convertToUv(p2, u2, v2);
r2 = p2.norm();

double u = u1 + (u2 - u1) * x;
double v = v1 + (v2 - v1) * x;
double r = r1 + (r2 - r1) * x;
u = (u - 0.5) * (2 * 3.1415);
v = v * 3.1415;

///  r=r*2; // TODO weg

return center + Vector3d(r * cos(u) * cos(v), r * sin(u) * cos(v), r * sin(v));
*/
}

Vector3d SphericalProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  double u, v;
  convertToUv(pt, u, v);
  return Vector3d(n[0], n[1], 0) * tcoord(u, v);
}

// ----------------------

int CylindricProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  Vector3d ptc = pt - center;
  u = 10 * (atan2(ptc[1], ptc[0]) / (2 * 3.1415926) + 0.5);
  v = pt[2];
  return 0;
}

Vector3d CylindricProjection::calcMidpoint(const Vector3d& a, const Vector3d& b, double x)
{
  return Vector3d(0, 0, 0);
}

Vector3d CylindricProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  double u, v;
  convertToUv(pt, u, v);
  return Vector3d(n[0], n[1], 0) * tcoord(u, v);
}

// ----------------------

int PlanarXProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  u = pt[1];
  v = pt[2];
  return 0;
}

Vector3d PlanarXProjection::calcMidpoint(const Vector3d& a, const Vector3d& b, double x)
{
  return Vector3d(0, 0, 0);
}

Vector3d PlanarXProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  double u, v;
  convertToUv(pt, u, v);
  return Vector3d(1, 0, 0) * tcoord(u, v);
}

// ----------------------

int PlanarYProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  u = pt[0];
  v = pt[2];
  return 0;
}
Vector3d PlanarYProjection::calcMidpoint(const Vector3d& a, const Vector3d& b, double x)
{
  return Vector3d(0, 0, 0);
}
Vector3d PlanarYProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  double u, v;
  convertToUv(pt, u, v);
  return Vector3d(0, 1, 0) * tcoord(u, v);
}

// ----------------------

int PlanarZProjection::convertToUv(const Vector3d& pt, double& u, double& v)
{
  u = pt[0];
  v = pt[1];
  return 0;
}
Vector3d PlanarZProjection::calcMidpoint(const Vector3d& a, const Vector3d& b, double x)
{
  printf("x=%g\n", x);
  return a + (b - a) * x;
}

Vector3d PlanarZProjection::calcDisplacement(const Vector3d& pt, const Vector3d& n)
{
  double u, v;
  convertToUv(pt, u, v);
  return Vector3d(0, 0, 1) * tcoord(u, v);
}
// ----------------------

std::unique_ptr<const Geometry> OversampleNode::createGeometry_sub(
  const std::shared_ptr<const PolySet>& ps) const
{
  auto ps_work = PolySetUtils::tessellate_faces(*ps);

  // calculate min and max
  Vector3d pmin, pmax;
  pmin = pmax = ps_work->vertices[0];
  for (const auto& pt : ps_work->vertices) {
    for (int i = 0; i < 3; i++) {
      if (pt[i] < pmin[i]) pmin[i] = pt[i];
      if (pt[i] > pmax[i]) pmax[i] = pt[i];
    }
  }
  Vector3d center = (pmin + pmax) / 2.0;

  BaseProjection *proj = nullptr;
  img_data_t texture;
  if (textureprojection != PROJECTION_NONE) texture = read_png_or_dat(this->texturefilename);
  switch (textureprojection) {
  case TRIPLANAR:
    proj = new TriPlanarProjection(texture, texturewidth, textureheight, texturedepth);
    break;
  case CUBIC: proj = new CubicProjection(texture, texturewidth, textureheight, texturedepth); break;
  case SPHERICAL:
    proj = new SphericalProjection(texture, texturewidth, textureheight, texturedepth, center);
    break;
  case CYLINDRIC:
    proj = new CylindricProjection(texture, texturewidth, textureheight, texturedepth, center);
    break;
  case PLANARX: proj = new PlanarXProjection(texture, texturewidth, textureheight, texturedepth); break;
  case PLANARY: proj = new PlanarYProjection(texture, texturewidth, textureheight, texturedepth); break;
  case PLANARZ: proj = new PlanarZProjection(texture, texturewidth, textureheight, texturedepth); break;
  }

  std::vector<Vector3d> normals;
  normals.reserve(ps_work->indices.size());
  std::vector<int> orig_id;
  orig_id.reserve(ps_work->indices.size());

  for (const auto& ind : ps_work->indices) {
    Vector3d p1 = ps->vertices[ind[0]];
    Vector3d p2 = ps->vertices[ind[1]];
    Vector3d p3 = ps->vertices[ind[2]];
    normals.push_back((p2 - p1).cross(p3 - p1).normalized());
  }
  for (int i = 0; i < ps_work->indices.size(); i++) orig_id.push_back(i);

  bool done = true;
  while (done) {
    done = false;
    PolySet ps_new(3);
    std::unordered_map<uint64_t, int> edges;
    ps_new.vertices = ps_work->vertices;
    ps_new.vertices.reserve(ps_work->vertices.size() * 2);
    ps_new.indices.reserve(ps_work->indices.size() * 4);
    std::vector<int> new_id;
    new_id.reserve(ps_work->indices.size() * 4);

    // decide which vertices to split
    for (const auto& tri : ps_work->indices) {
      int ind_old = tri[2];
      for (int ind : tri) {
        double dist2 = (ps_work->vertices[ind_old] - ps_work->vertices[ind]).squaredNorm();
        if (dist2 > size * size) {
          uint64_t key = ((uint64_t)std::min(ind, ind_old) << 32) | std::max(ind, ind_old);
          auto it = edges.find(key);
          if (it == edges.end()) {
            Vector3d pmid;
            if (proj != nullptr)
              pmid = proj->calcMidpoint(ps_work->vertices[ind_old], ps_work->vertices[ind]);
            else pmid = (ps_work->vertices[ind_old] + ps_work->vertices[ind]) / 2;
            //            printf("pmid is %g/%g/%g\n", pmid[0], pmid[1], pmid[2]);

            int newind = ps_new.vertices.size();
            ps_new.vertices.push_back(pmid);
            edges[key] = newind;
            done = true;
          }
        }
        ind_old = ind;
      }
    }
    // build new triangles from split vertices
    for (int i = 0; i < ps_work->indices.size(); i++) {
      const auto& tri = ps_work->indices[i];
      int ind_old = tri[2];
      int tri_mid[3];
      for (int i = 0; i < 3; i++) {
        int ind = tri[i];
        uint64_t key = ((uint64_t)std::min(ind, ind_old) << 32) | std::max(ind, ind_old);
        auto it = edges.find(key);
        tri_mid[(i + 2) % 3] = (it != edges.end()) ? it->second : -1;
        ind_old = ind;
      }
      int a = 0;
      switch (((tri_mid[0] >= 0) ? 1 : 0) | ((tri_mid[1] >= 0) ? 2 : 0) | ((tri_mid[2] >= 0) ? 4 : 0)) {
      case 0:
        ps_new.indices.push_back(tri);
        a = 1;
        break;
      case 1:
        ps_new.indices.push_back({tri[0], tri_mid[0], tri[2]});
        ps_new.indices.push_back({tri[2], tri_mid[0], tri[1]});
        a = 2;
        break;
      case 2:
        ps_new.indices.push_back({tri[1], tri_mid[1], tri[0]});
        ps_new.indices.push_back({tri[0], tri_mid[1], tri[2]});
        a = 2;
        break;
      case 3:
        ps_new.indices.push_back({tri[0], tri_mid[0], tri[2]});
        ps_new.indices.push_back({tri[2], tri_mid[0], tri_mid[1]});
        ps_new.indices.push_back({tri_mid[1], tri_mid[0], tri[1]});
        a = 3;
        break;
      case 4:
        ps_new.indices.push_back({tri[2], tri_mid[2], tri[1]});
        ps_new.indices.push_back({tri[1], tri_mid[2], tri[0]});
        a = 2;
        break;
      case 5:
        ps_new.indices.push_back({tri[2], tri_mid[2], tri[1]});
        ps_new.indices.push_back({tri[1], tri_mid[2], tri_mid[0]});
        ps_new.indices.push_back({tri_mid[0], tri_mid[2], tri[0]});
        a = 3;
        break;
      case 6:
        ps_new.indices.push_back({tri[1], tri_mid[1], tri[0]});
        ps_new.indices.push_back({tri[0], tri_mid[1], tri_mid[2]});
        ps_new.indices.push_back({tri_mid[2], tri_mid[1], tri[2]});
        a = 3;
        break;
      case 7:
        ps_new.indices.push_back({tri[0], tri_mid[0], tri_mid[2]});
        ps_new.indices.push_back({tri[1], tri_mid[1], tri_mid[0]});
        ps_new.indices.push_back({tri[2], tri_mid[2], tri_mid[1]});
        ps_new.indices.push_back({tri_mid[0], tri_mid[1], tri_mid[2]});
        a = 4;
        break;
      }
      for (int j = 0; j < a; j++) new_id.push_back(orig_id[i]);
    }
    if (done) {
      *ps_work = ps_new;
      orig_id = new_id;
    }

    // now check which edges can be flipped
    std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey>> edgeDb;
    edgeDb = createEdgeDb(ps_work->indices);
    for (auto it = edgeDb.begin(); it != edgeDb.end(); it++) {
      const EdgeKey& key = it->first;
      const EdgeVal& val = it->second;

      int left = -1, right = -1, afar = -1, bfar = -1, tmp;
      for (int i = 0; i < 3; i++) {
        tmp = ps_work->indices[val.facea][i];
        if (tmp != key.ind1 && tmp != key.ind2) {
          afar = ps_work->indices[val.facea][i];
          left = ps_work->indices[val.facea][(i + 1) % 3];
          right = ps_work->indices[val.facea][(i + 2) % 3];
        }
        tmp = ps_work->indices[val.faceb][i];
        if (tmp != key.ind1 && tmp != key.ind2) {
          bfar = ps_work->indices[val.faceb][i];
        }
      }
      const Vector3d& pleft = ps_work->vertices[left];
      const Vector3d& pright = ps_work->vertices[right];
      const Vector3d& pafar = ps_work->vertices[afar];
      const Vector3d& pbfar = ps_work->vertices[bfar];
      const Vector3d phor = pright - pleft;
      const Vector3d pver = pafar - pbfar;

      do {
        if (pver.squaredNorm() >= phor.squaredNorm() * 0.990) break;
        EdgeKey opp1k(left, bfar);
        auto it = edgeDb.find(opp1k);
        if (it == edgeDb.end()) break;
        EdgeVal& opp1 = it->second;

        EdgeKey opp2k(right, afar);
        it = edgeDb.find(opp2k);
        if (it == edgeDb.end()) break;
        EdgeVal& opp2 = it->second;

        Vector3d n = (pafar - pleft).cross(phor);
        if (fabs(n.dot(pafar) - n.dot(pbfar)) > 1e-3) break;  // same level

        if ((pleft - pbfar).cross(pver).dot(n) <= 1e-3) break;   // triangle flip
        if ((pbfar - pright).cross(pver).dot(n) <= 1e-3) break;  // triangle flip

        EdgeKey keyt(bfar, afar);

        for (int i = 0; i < 3; i++) {
          if (ps_work->indices[val.facea][i] == right) ps_work->indices[val.facea][i] = bfar;
          if (ps_work->indices[val.faceb][i] == left) ps_work->indices[val.faceb][i] = afar;
        }
        if (opp1.facea == val.faceb) opp1.facea = val.facea;
        if (opp1.faceb == val.faceb) opp1.faceb = val.facea;
        if (opp2.facea == val.facea) opp2.facea = val.faceb;
        if (opp2.faceb == val.facea) opp2.faceb = val.faceb;

      } while (0);
    }
  }

  if (textureprojection != PROJECTION_NONE && texturefilename.size() > 0) {
    // now apply texture to all vertices
    // create  vertex-to-triangle mapping (beta)
    std::vector<int> vert2tri;
    vert2tri.reserve(ps_work->vertices.size());
    for (int i = 0; i < ps_work->vertices.size(); i++) vert2tri.push_back(0);
    for (int i = 0; i < ps_work->indices.size(); i++) {
      for (int j = 0; j < 3; j++) {
        vert2tri[ps_work->indices[i][j]] = i;
      }
    }
    for (int i = 0; i < ps_work->vertices.size(); i++) {
      Vector3d& n = normals[orig_id[vert2tri[i]]];
      Vector3d& pt = ps_work->vertices[i];
      pt = pt + proj->calcDisplacement(pt, n);
    }
    if (proj != nullptr) delete proj;
  }
  return std::make_unique<PolySet>(*ps_work);
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
  return createGeometry_sub(ps);
}

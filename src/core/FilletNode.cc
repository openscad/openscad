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

#include "FilletNode.h"
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
#include <src/geometry/PolySetUtils.h>

struct SearchReplace {
  int pol;
  int search;
  IndexedFace replace;
};

typedef std::vector<int> intList;

bool list_included(const std::vector<int>& list, int needle)
{
  for (size_t i = 0; i < list.size(); i++) {
    if (list[i] == needle) return true;
  }
  return false;
}
std::shared_ptr<const PolySet> childToPolySet(std::shared_ptr<AbstractNode> child)
{
  Tree tree(child, "");
  GeometryEvaluator geomevaluator(tree);
  std::shared_ptr<const Geometry> geom = geomevaluator.evaluateGeometry(*tree.root(), true);
  std::shared_ptr<const PolySet> ps;
  return PolySetUtils::getGeometryAsPolySet(geom);
}

int point_in_polyhedron(const PolySet& ps, const Vector3d& pt)
{
  // polygons are clockwise
  int cuts = 0;
  Vector3d vc(1, 0, 0);
  Vector3d res;
  for (size_t i = 0; i < ps.indices.size(); i++) {
    const IndexedFace& f = ps.indices[i];
    Vector3d va = ps.vertices[f[1]] - ps.vertices[f[0]];
    Vector3d vb = ps.vertices[f[2]] - ps.vertices[f[0]];
    if (linsystem(va, vb, vc, pt - ps.vertices[f[0]], res, nullptr)) continue;
    if (res[2] < 0) continue;
    if (res[0] >= -1e-6 && res[1] > -1e-6 && res[0] + res[1] < 1 + 1e-6) {
      if (res[0] > 0 && res[1] > 0 && res[0] + res[1] < 1) cuts += 2;
      if (fabs(res[0]) < 1e-6) cuts++;
      if (fabs(res[1]) < 1e-6) cuts++;
      if (fabs(res[0] + res[1] - 1) < 1e-6) cuts++;
    }
  }
  cuts /= 2;
  return cuts & 1;
}

// Credit: inphase Ryan Colyer
Vector3d Bezier(double t, Vector3d a, Vector3d b, Vector3d c)
{
  return (a * (1 - t) + b * t) * (1 - t) + (b * (1 - t) + c * t) * t;  // TODO improve
}

void bezier_patch(PolySetBuilder& builder, Vector3d center, Vector3d dir[3], int concave_1,
                  int concave_2, int concave_3, int N)
{
  if ((dir[1].cross(dir[0])).dot(dir[2]) < 0) {
    Vector3d tmp = dir[0];
    dir[0] = dir[1];
    dir[1] = tmp;
  }
  Vector3d xdir = dir[0].normalized();
  Vector3d ydir = dir[1].normalized();
  Vector3d zdir = dir[2].normalized();

  // zdir shall look upwards

  Matrix3d mat;
  mat << xdir[0], ydir[0], zdir[0], xdir[1], ydir[1], zdir[1], xdir[2], ydir[2], zdir[2];

  xdir = Vector3d(1, 0, 0) * dir[0].norm();
  ydir = Vector3d(0, 1, 0) * dir[1].norm();
  zdir = Vector3d(0, 0, 1) * dir[2].norm();

  // now use matrices to transform the vectors into std orientation
  //
  //  N = floor(N/2)*2 + 1;
  Vector3d pt;
  std::vector<Vector3d> points_xz;
  std::vector<Vector3d> points_yz;
  for (int i = 0; i < N; i++) {
    double t = (double)i / (double)(N - 1);
    points_xz.push_back(
      Bezier(t, xdir, xdir + zdir, zdir + 2 * (concave_1 + concave_2) * (xdir + ydir)));
    points_yz.push_back(
      Bezier(t, ydir, ydir + zdir, zdir + 2 * (concave_1 + concave_2) * (xdir + ydir)));
  }

  std::vector<int> points;
  for (int i = 0; i < N; i++) {
    if (i == N - 1) {
      pt = zdir + 2 * (concave_1 + concave_2) * (xdir + ydir);
      pt = mat * pt;
      points.push_back(builder.vertexIndex(pt + center));
    } else {
      int M = N - i;
      for (int j = 0; j < M; j++) {
        int k;
        if (concave_1 == 1 || concave_3 == 1) k = j;
        else k = M - 1 - j;
        double t2 = (double)k / (double)(M - 1);
        pt = Bezier(t2, points_xz[i], Vector3d(points_xz[i][0], points_yz[i][1], points_xz[i][2]),
                    points_yz[i]);
        pt = mat * pt;
        points.push_back(builder.vertexIndex(center + pt));
      }
    }
  }
  // total points = N*(N-1)/2
  int off = 0;
  for (int i = 0; i < N - 1; i++) {  // Zeile i, i-1
    int off_new = off + (N - i);
    for (int j = 0; j < N - i - 1; j++) {
      builder.appendPolygon({points[off + j], points[off + j + 1], points[off_new + j]});
      if (j < N - i - 2) {
        builder.appendPolygon({points[off + j + 1], points[off_new + j + 1], points[off_new + j]});
      }
    }
    off = off_new;
  }
}

void debug_pt(const char *msg, Vector3d pt) { printf("%s %g/%g/%g\n", msg, pt[0], pt[1], pt[2]); }

std::unique_ptr<const Geometry> createFilletInt(std::shared_ptr<const PolySet> ps,
                                                std::vector<bool> corner_selected, double r_, int bn,
                                                double minang)
{
  double cos_minang = cos(minang * 3.1415 / 180.0);
  std::vector<Vector4d> normals, newnormals;
  std::vector<int> faceParents;
  normals = calcTriangleNormals(ps->vertices, ps->indices);
  std::vector<IndexedFace> merged =
    mergeTriangles(ps->indices, normals, newnormals, faceParents, ps->vertices);

  if (bn < 2) bn = 2;
  // Create vertex2face db
  auto vertices_copy = ps->vertices;

  bool improved = false;
  std::unordered_map<EdgeKey, EdgeVal, boost::hash<EdgeKey>> edge_db;
  std::vector<intList> polinds, polposs;

  std::vector<std::vector<int>> corner_rounds;
  do {
    improved = false;  // fix short edges until happy
    std::vector<int> lockouts;

    polinds.clear();
    polposs.clear();
    intList empty;
    for (size_t i = 0; i < vertices_copy.size(); i++) {
      polinds.push_back(empty);
      polposs.push_back(empty);
    }
    for (size_t i = 0; i < merged.size(); i++) {
      for (size_t j = 0; j < merged[i].size(); j++) {
        int ind = merged[i][j];
        polinds[ind].push_back(i);
        polposs[ind].push_back(j);
      }
    }

    // create Edge DB

    edge_db = createEdgeDb(merged);

    // which rounded edges in a corner coner_rounds[vert]=[other_verts]
    corner_rounds.clear();
    for (size_t i = 0; i < vertices_copy.size(); i++) corner_rounds.push_back(empty);

    for (auto& e : edge_db) {
      if (corner_selected[e.first.ind1] && corner_selected[e.first.ind2]) {
        assert(e.second.facea >= 0);
        assert(e.second.faceb >= 0);
        auto& facea = merged[e.second.facea];
        auto& faceb = merged[e.second.faceb];
        Vector3d fan = calcTriangleNormal(vertices_copy, facea).head<3>();
        Vector3d fbn = calcTriangleNormal(vertices_copy, faceb).head<3>();
        double d = fan.dot(fbn);
        e.second.sel = 0;
        if (d >= cos_minang) continue;  // dont create facets when the angle conner is too small
        if (polinds[e.first.ind1].size() != 3) continue;  // start must be 3edge corner
        if (polinds[e.first.ind2].size() != 3) continue;  // start must be 3edge corner

        e.second.sel = 1;
        corner_rounds[e.first.ind1].push_back(e.first.ind2);
        corner_rounds[e.first.ind2].push_back(e.first.ind1);
      }
    }
    /* TODO activate

        // eliminate  too short edges by extrapolating the neighboring edges
        for(auto &e: edge_db) {
          if(!e.second.sel) continue;
          Vector3d line = vertices_copy[e.first.ind1] - vertices_copy[e.first.ind2];
          if(line.norm() < 2*r_) {

            int a_prev=-1, a_next=-1;
            int b_prev=-1, b_next=-1;

            if(std::find(lockouts.begin(), lockouts.end(), e.first.ind1) != lockouts.end()) continue;
            if(std::find(lockouts.begin(), lockouts.end(), e.first.ind2) != lockouts.end()) continue;
            auto &facea = merged[e.second.facea];
            int na=facea.size();
            for(int i=0;i<na;i++) {
              if(facea[i] == e.first.ind1){
                a_prev = facea[(i+na-1)%na];
                a_next = facea[(i+na+2)%na];
              }
            }

            auto &faceb = merged[e.second.faceb];
            int nb=faceb.size();
            for(int i=0;i<nb;i++) {
              if(faceb[i] == e.first.ind2){
                b_prev = faceb[(i+nb-1)%nb];
                b_next = faceb[(i+nb+2)%nb];
              }
            }

            if(std::find(lockouts.begin(), lockouts.end(), a_prev) != lockouts.end()) continue;
            if(std::find(lockouts.begin(), lockouts.end(), a_next) != lockouts.end()) continue;
            if(std::find(lockouts.begin(), lockouts.end(), b_prev) != lockouts.end()) continue;
            if(std::find(lockouts.begin(), lockouts.end(), b_next) != lockouts.end()) continue;

            // is it safe to take the bigger face ?
            int commonfaceind=-1, faceind1=-1, faceind2=-1;
            EdgeKey ek1, ek2;
            if(nb > na) {
              commonfaceind=e.second.faceb; // TODO b hat die richtigen punkte
              ek1 = EdgeKey(b_prev, e.first.ind2);
              ek2 = EdgeKey(e.first.ind1, b_next);
            } else { // na  > nb)
              commonfaceind=e.second.facea; // TODO b hat die richtigen punkte
              ek1 = EdgeKey(a_prev, e.first.ind1);
              ek2 = EdgeKey(e.first.ind2, a_next);
            }

            if(edge_db.count(ek1)) {
              auto &ev1 = edge_db.at(ek1);
              if(ev1.facea == commonfaceind) faceind1= ev1.faceb;
              if(ev1.faceb == commonfaceind) faceind1= ev1.facea;
            }

            // find opposite of e.first.ind1, b_next)
            if(edge_db.count(ek2)) {
              auto &ev2 = edge_db.at(ek2);
              if(ev2.facea == commonfaceind) faceind2= ev2.faceb;
              if(ev2.faceb == commonfaceind) faceind2= ev2.facea;
            }

            Vector3d fn1 =calcTriangleNormal(vertices_copy, merged[commonfaceind]).head<3>();
            Vector3d fn2 =calcTriangleNormal(vertices_copy, merged[faceind1]).head<3>();
            Vector3d fn3 =calcTriangleNormal(vertices_copy, merged[faceind2]).head<3>();

            Vector3d fp1 = vertices_copy[merged[commonfaceind][0]];
            Vector3d fp2 = vertices_copy[merged[faceind1][0]];
            Vector3d fp3 = vertices_copy[merged[faceind2][0]];
            Vector3d ptcut;
            if(cut_face_face_face(fp1, fn1, fp2, fn2, fp3, fn3, ptcut,nullptr)) {
              printf("Error during cutting\n");
              e.second.sel=0;
              continue;
            }
            //
            // change is going to happen
            vertices_copy[e.first.ind1]=ptcut;
            lockouts.push_back(ek1.ind1);
            lockouts.push_back(ek1.ind2);
            lockouts.push_back(ek2.ind1);
            lockouts.push_back(ek2.ind2);

            for(int j=0;j<merged.size();j++) {
              auto &tri = merged[j];
              int n = tri.size();
              int dupind=-1;
              for(int i=0;i<n;i++)
              {
                if(tri[i] == e.first.ind2){
                  tri[i] =e.first.ind1;
                  if(tri[(i+1)%n] == e.first.ind1 || tri[(i+n-1)%n] == e.first.ind1) {
                    dupind=i;
                  }
                }
              }
              if(dupind != -1) {
                IndexedFace tri_new;
                for(int i=0;i<dupind;i++) tri_new.push_back(tri[i]);
                for(int i=dupind+1;i<n;i++) tri_new.push_back(tri[i]);
                tri = tri_new;
                n--;
              }
              if(n < 3) {
                    merged.erase(merged.begin()+j);
                    j--;
              }
            }
            improved=true;
          // TODO lockout
          }

        }
    */
  } while (improved == true);

  // start builder with existing vertices to have VertexIndex available
  //
  PolySetBuilder builder;
  for (size_t i = 0; i < vertices_copy.size(); i++) {
    builder.vertexIndex(vertices_copy[i]);  // allocate all vertices in the right order
  }

  SearchReplace s;
  std::vector<SearchReplace> sp;

  // plan fillets of all edges now
  for (auto& e : edge_db) {
    if (e.second.sel == 1) {
      Vector3d p1 = vertices_copy[e.first.ind1];  // both ends of the selected edge
      Vector3d p2 = vertices_copy[e.first.ind2];
      Vector3d p1org = p1, p2org = p2;
      Vector3d dir = p2 - p1;
      if (corner_rounds[e.first.ind1].size() >= 3) p1 += dir.normalized() * r_;
      if (corner_rounds[e.first.ind2].size() >= 3) p2 -= dir.normalized() * r_;
      dir = dir.normalized();  // TODO
      auto& facea = merged[e.second.facea];
      auto& faceb = merged[e.second.faceb];
      //

      int facean = facea.size();
      int facebn = faceb.size();
      double fanf = (faceParents[e.second.facea] != -1) ? -1 : 1;  // is the edge part of a hole
      double fbnf = (faceParents[e.second.faceb] != -1) ? -1 : 1;
      Vector3d fan = calcTriangleNormal(vertices_copy, facea).head<3>();
      Vector3d fbn = calcTriangleNormal(vertices_copy, faceb).head<3>();

      // A 1st side of the edge
      // B 2nd face of the edge
      int indposao, indposbo, indposai, indposbi;
      Vector3d unit;

      indposao = facea[(e.second.posa + facean - 1) % facean];  // o away from edge
      indposai = facea[(e.second.posa + 1) % facean];           // i on edge

      indposbo = faceb[(e.second.posb + 2) % facebn];
      indposbi = faceb[e.second.posb];

      Vector3d e_fa1 = (vertices_copy[indposao] - vertices_copy[facea[e.second.posa]]).normalized() *
                       fanf;  // Facea neben ind1
      Vector3d e_fa1p =
        (vertices_copy[indposai] - vertices_copy[facea[e.second.posa]]) * fanf;  // Face1 nahe  richtung
                                                                                 //
      Vector3d e_fb1 =
        (vertices_copy[indposbo] - vertices_copy[faceb[(e.second.posb + 1) % facebn]]).normalized() *
        fbnf;  // Faceb neben ind1
      Vector3d e_fb1p =
        (vertices_copy[indposbi] - vertices_copy[faceb[(e.second.posb + 1) % facebn]]) * fbnf;

      if (corner_rounds[e.first.ind1].size() == 2) {
        double a = (e_fb1.cross(e_fa1)).dot(dir);
        double b = (fan.cross(fbn)).dot(e_fa1p) * fanf * fbnf;
        if (list_included(corner_rounds[e.first.ind1], indposao)) {
          double ang = (dir).dot(e_fa1.normalized());
          e_fa1 += dir * fanf;
          if (a * b < 0) e_fa1 = -e_fa1 * fanf;
          e_fa1 /= sqrt(1 - ang * ang);
        }

        if (list_included(corner_rounds[e.first.ind1], indposbo)) {
          double ang = (dir).dot(e_fb1.normalized());
          e_fb1 += dir * fbnf;
          if (a * b < 0) e_fb1 = -e_fb1 * fbnf;
          e_fb1 /= sqrt(1 - ang * ang);
        }
      }

      if (corner_rounds[e.first.ind1].size() == 3) {
        if ((fbn.cross(fan)).dot(e_fa1p) < 0 || (fbn.cross(fan)).dot(e_fb1p) < 0) {
          if ((e_fa1p.cross(e_fa1)).dot(fan) * fanf < 0) {
            e_fa1 = -e_fa1 * fanf - 2 * (p2org - p1org).normalized();
          }
          if ((e_fb1p.cross(e_fb1)).dot(fbn) * fbnf > 0) {
            e_fb1 = -e_fb1 * fbnf - 2 * (p2org - p1org).normalized();
          }
        }
        if ((fbn.cross(fan)).dot(e_fb1p) > 0) {
          if ((e_fa1p.cross(e_fa1)).dot(fan) * fanf > 0 && (e_fa1p.cross(e_fb1)).dot(fbn) * fbnf > 0) {
            e_fb1 = -e_fb1 * fbnf - 2 * dir;
          }
          if ((e_fb1p.cross(e_fb1)).dot(fbn) * fbnf < 0 && (e_fb1p.cross(e_fa1)).dot(fan) * fanf < 0) {
            e_fa1 = -e_fa1 * fanf - 2 * dir;
          }
        }
      }
      e_fa1 *= r_;
      e_fb1 *= r_;

      indposao = facea[(e.second.posa + 2) % facean];
      indposai = facea[e.second.posa];

      indposbo = faceb[(e.second.posb + facebn - 1) % facebn];
      indposbi = faceb[(e.second.posb + 1) % facebn];

      Vector3d e_fa2 =
        (vertices_copy[indposao] - vertices_copy[facea[(e.second.posa + 1) % facean]]).normalized() *
        fanf;  // Face1 entfernte richtung
      Vector3d e_fa2p = (vertices_copy[indposai] - vertices_copy[facea[(e.second.posa + 1) % facean]]) *
                        fanf;  // Face1 entfernte richtung
                               //
      Vector3d e_fb2 =
        (vertices_copy[indposbo] - vertices_copy[faceb[(e.second.posb + 0) % facebn]]).normalized() *
        fbnf;  // Face2 entfernte Rcithung
      Vector3d e_fb2p = (vertices_copy[indposbi] - vertices_copy[faceb[(e.second.posb + 0) % facebn]]) *
                        fbnf;  // Face2 entfernte Rcithung

      //
      if (corner_rounds[e.first.ind2].size() == 2) {
        double a = (e_fb2.cross(e_fa2)).dot(dir);
        double b = (fan.cross(fbn)).dot(e_fa2p) * fanf * fbnf;
        double c = (fan.cross(fbn)).dot(dir);
        if (list_included(corner_rounds[e.first.ind2], indposao)) {
          double ang = (dir).dot(e_fa2.normalized());
          e_fa2 -= dir * fanf;
          if (a * b > 0) e_fa2 = -e_fa2 * fanf;
          e_fa2 /= sqrt(1 - ang * ang);
        }

        if (list_included(corner_rounds[e.first.ind2], indposbo)) {
          double ang = (dir).dot(e_fb2.normalized());
          e_fb2 -= dir * fbnf;
          if (a * b > 0) e_fb2 = -e_fb2 * fbnf;
          e_fb2 /= sqrt(1 - ang * ang);
        }
      }

      if (corner_rounds[e.first.ind2].size() == 3) {
        if (-(fbn.cross(fan)).dot(e_fa2p) < 0 || -(fbn.cross(fan)).dot(e_fb2p) < 0) {
          if (-(e_fa2p.cross(e_fa2)).dot(fan) * fanf < 0) {
            e_fa2 = -e_fa2 * fanf + 2 * dir;
          }
          if (-(e_fb2p.cross(e_fb2)).dot(fbn) * fbnf > 0) {
            e_fb2 = -e_fb2 * fbnf + 2 * dir;
          }
        }
        if (/* -(fbn.cross(fan)).dot(e_fa2p) > 0 || */ -(fbn.cross(fan)).dot(e_fb2p) > 0) {
          if (-(e_fb2p.cross(e_fb2)).dot(fbn) * fbnf < 0 && (e_fb2p.cross(e_fa2)).dot(fan) * fanf > 0) {
            e_fa2 = -e_fa2 * fanf + 2 * dir;  // laengs links
          }
          if (-(e_fa2p.cross(e_fa2)).dot(fan) * fanf > 0 && (e_fa2p.cross(e_fb2)).dot(fbn) * fbnf < 0) {
            e_fb2 = -e_fb2 * fbnf + 2 * dir;
          }
        }
      }

      e_fa2 *= r_;
      e_fb2 *= r_;

      // Calculate bezier patches
      for (int i = 0; i < bn; i++) {
        double f = (double)i / (double)(bn - 1);  // from 0 to 1
        e.second.bez1.push_back(
          builder.vertexIndex(p1 + e_fa1 - 2 * f * e_fa1 + f * f * (e_fa1 + e_fb1)));
        e.second.bez2.push_back(
          builder.vertexIndex(p2 + e_fa2 - 2 * f * e_fa2 + f * f * (e_fa2 + e_fb2)));
      }
      s.pol = e.second.facea;  // laengsseite1
      s.search = e.first.ind1;
      s.replace = {e.second.bez1[0]};
      sp.push_back(s);
      s.pol = e.second.facea;  // laengsseite1
      s.search = e.first.ind2;
      s.replace = {e.second.bez2[0]};
      sp.push_back(s);

      s.pol = e.second.faceb;  // laengsseite2
      s.search = e.first.ind2;
      s.replace = {e.second.bez2[bn - 1]};
      sp.push_back(s);
      s.pol = e.second.faceb;  // laengsseite2
      s.search = e.first.ind1;
      s.replace = {e.second.bez1[bn - 1]};
      sp.push_back(s);

      // stirnseite 1
      if (corner_rounds[e.first.ind1].size() == 1) {
        for (size_t i = 0; i < polinds[e.first.ind1].size(); i++) {
          int faceid = polinds[e.first.ind1][i];
          if (faceid == e.second.facea) continue;
          if (faceid == e.second.faceb) continue;
          s.pol = faceid;  // stirnseite1
          s.search = e.first.ind1;
          s.replace = {e.second.bez1};
          std::reverse(s.replace.begin(), s.replace.end());
          sp.push_back(s);
        }
      }

      // stirnseite2
      if (corner_rounds[e.first.ind2].size() == 1) {
        for (size_t i = 0; i < polinds[e.first.ind2].size(); i++) {
          int faceid = polinds[e.first.ind2][i];
          if (faceid == e.second.facea) continue;
          if (faceid == e.second.faceb) continue;
          s.pol = faceid;  // stirnseite2
          s.search = e.first.ind2;
          s.replace = {e.second.bez2};
          sp.push_back(s);
        }
      }
      //     printf("\nNum=%d\n",debug);
      //     printf("P : %g/%g/%g EA: %g/%g/%g EB %g/%g/%g\n",p1[0], p1[1], p1[2], e_fa1[0], e_fa1[1],
      //     e_fa1[2], e_fb1[0], e_fb1[1], e_fb1[2]); printf("P : %g/%g/%g EA: %g/%g/%g EB
      //     %g/%g/%g\n",p2[0], p2[1], p2[2], e_fa2[0], e_fa2[1], e_fa2[2], e_fb2[0], e_fb2[1],
      //     e_fb2[2]);
    }
  }
  // copy modified faces
  std::vector<IndexedFace> newfaces;
  for (size_t i = 0; i < merged.size(); i++) {
    const IndexedFace& face = merged[i];
    IndexedFace newface;
    for (size_t j = 0; j < face.size(); j++) {
      int ind = face[j];
      newface.push_back(ind);
    }
    int fn = newface.size();
    // does newface need any mods ?
    for (size_t j = 0; j < sp.size(); j++) {  // TODO effektiver, sp sortiren und 0 groesser machen
      if ((size_t)sp[j].pol == i) {
        int needle = sp[j].search;
        for (int k = 0; k < fn; k++) {  // all possible shifts
          if (newface[k] == needle) {
            // match bei shift k gefunden
            IndexedFace tmp = sp[j].replace;
            for (int l = 0; l < fn - 1; l++) {
              tmp.push_back(newface[(k + 1 + l) % fn]);
            }
            newface = tmp;
            fn = newface.size();
            break;
          }
        }
      }
    }

    newfaces.push_back(newface);
  }
  std::vector<Vector3d> vertices;
  builder.copyVertices(vertices);
  std::vector<Vector3f> verticesFloat;
  for (const auto& v : vertices) verticesFloat.push_back(v.cast<float>());

  for (size_t i = 0; i < newfaces.size(); i++) {
    // tessellate first with holes // search all holes
    if (faceParents[i] != -1) continue;
    std::vector<IndexedFace> faces;
    faces.push_back(newfaces[i]);
    for (size_t j = 0; j < newfaces.size(); j++)
      if ((size_t)faceParents[j] == i) faces.push_back(newfaces[j]);
    //    if(faces.size() >1 ) continue;
    std::vector<IndexedTriangle> triangles;
    Vector3f norm(newnormals[i][0], newnormals[i][1], newnormals[i][2]);
    GeometryUtils::tessellatePolygonWithHoles(verticesFloat, faces, triangles, &norm);
    for (const auto& t : triangles) {
      builder.appendPolygon({t[0], t[1], t[2]});
    }
  }

  // add Rounded edges
  for (auto& e : edge_db) {
    if (e.second.sel == 1) {
      // now create the faces
      for (int i = 0; i < bn - 1; i++) {
        builder.appendPolygon(
          {e.second.bez1[i], e.second.bez1[i + 1], e.second.bez2[i + 1], e.second.bez2[i]});
      }
    }
  }
  // add missing 3 corner patches
  //
  for (size_t i = 0; i < vertices_copy.size(); i++) {
    if (corner_rounds[i].size() > 3) {
      printf("corner %ld not possible\n", i);
    } else if (corner_rounds[i].size() == 3) {
      // now get the right ordering of corner_rounds[i]
      IndexedFace face[3];
      Vector3d facenorm[3];
      for (int j = 0; j < 3; j++) {
        face[j] = merged[polinds[i][j]];
        facenorm[j] = calcTriangleNormal(vertices_copy, face[j]).head<3>();
        if (faceParents[polinds[i][j]] != -1) facenorm[j] = -facenorm[j];
      }

      int facebeg[3];
      int faceend[3];
      for (int j = 0; j < 3; j++) {
        facebeg[j] = face[j][(polposs[i][j] + face[j].size() - 1) % face[j].size()];
        faceend[j] = face[j][(polposs[i][j] + 1) % face[j].size()];
      }

      std::vector<int> angle;
      std::vector<Vector3d> dir;
      Vector3d x;
      if (faceend[0] == facebeg[1]) {  // 0,1,2
        x = vertices_copy[faceend[1]] - vertices_copy[i];
        dir.push_back(x.normalized() * r_);
        angle.push_back(
          (facenorm[1].cross(facenorm[2])).dot(vertices_copy[faceend[1]] - vertices_copy[i]) > 0 ? 1
                                                                                                 : -1);
        x = vertices_copy[faceend[2]] - vertices_copy[i];
        dir.push_back(x.normalized() * r_);
        angle.push_back(
          (facenorm[2].cross(facenorm[0])).dot(vertices_copy[faceend[2]] - vertices_copy[i]) > 0 ? 1
                                                                                                 : -1);
        x = vertices_copy[faceend[0]] - vertices_copy[i];
        dir.push_back(x.normalized() * r_);
        angle.push_back(
          (facenorm[0].cross(facenorm[1])).dot(vertices_copy[faceend[0]] - vertices_copy[i]) > 0 ? 1
                                                                                                 : -1);
      } else if (faceend[0] == facebeg[2]) {
        x = vertices_copy[faceend[2]] - vertices_copy[i];
        dir.push_back(x.normalized() * r_);
        angle.push_back(
          (facenorm[2].cross(facenorm[1])).dot(vertices_copy[faceend[2]] - vertices_copy[i]) > 0 ? 1
                                                                                                 : -1);
        x = vertices_copy[faceend[0]] - vertices_copy[i];
        dir.push_back(x.normalized() * r_);
        angle.push_back(
          (facenorm[0].cross(facenorm[2])).dot(vertices_copy[faceend[0]] - vertices_copy[i]) > 0 ? 1
                                                                                                 : -1);
        x = vertices_copy[faceend[1]] - vertices_copy[i];
        dir.push_back(x.normalized() * r_);
        angle.push_back(
          (facenorm[1].cross(facenorm[0])).dot(vertices_copy[faceend[1]] - vertices_copy[i]) > 0 ? 1
                                                                                                 : -1);
      } else assert(0);
      int conc1 = -1, conc2 = -1, conc3 = -1;
      int dirshift = -1;
      Vector3d pdir[3];
      if (angle[0] == -1 && angle[1] == -1 && angle[2] == -1) {
        dirshift = 0;
        conc1 = 0;
        conc2 = 0;
        conc3 = 1;
      }
      if (angle[0] == -1 && angle[1] == -1 && angle[2] == 1) {
        dirshift = 0;
        conc1 = 0;
        conc2 = 1;
        conc3 = 0;
      }
      if (angle[0] == -1 && angle[1] == 1 && angle[2] == -1) {
        dirshift = 2;
        conc1 = 0;
        conc2 = 1;
        conc3 = 0;
      }
      if (angle[0] == -1 && angle[1] == 1 && angle[2] == 1) {
        dirshift = 1;
        conc1 = 1;
        conc2 = 0;
        conc3 = 0;
      }
      if (angle[0] == 1 && angle[1] == -1 && angle[2] == -1) {
        dirshift = 1;
        conc1 = 0;
        conc2 = 1;
        conc3 = 0;
      }
      if (angle[0] == 1 && angle[1] == -1 && angle[2] == 1) {
        dirshift = 2;
        conc1 = 1;
        conc2 = 0;
        conc3 = 0;
      }
      if (angle[0] == 1 && angle[1] == 1 && angle[2] == -1) {
        dirshift = 0;
        conc1 = 1;
        conc2 = 0;
        conc3 = 0;
      }
      if (angle[0] == 1 && angle[1] == 1 && angle[2] == 1) {
        dirshift = 0;
        conc1 = 0;
        conc2 = 0;
        conc3 = 0;
      }
      if (dirshift != -1) {
        for (int i = 0; i < 3; i++) {
          pdir[i] = -dir[(i + dirshift) % 3];
        }
        bezier_patch(builder, ps->vertices[i] - pdir[0] - pdir[1] - pdir[2], pdir, conc1, conc2, conc3,
                     bn);
      }
    }
  }
  //
  auto result = builder.build();

  return result;
}

std::unique_ptr<const Geometry> FilletNode::createGeometry() const
{
  std::shared_ptr<const PolySet> ps;
  std::vector<bool> corner_selected;
  if (this->children.size() >= 1) {
    ps = childToPolySet(this->children[0]);
    if (ps == nullptr) return std::unique_ptr<PolySet>();
  } else return std::unique_ptr<PolySet>();

  if (this->children.size() >= 2) {
    std::shared_ptr<const PolySet> sel = childToPolySet(this->children[1]);
    if (sel != nullptr) {
      auto sel_tess = PolySetUtils::tessellate_faces(*sel);
      for (size_t i = 0; i < ps->vertices.size(); i++) {
        corner_selected.push_back(point_in_polyhedron(*sel_tess, ps->vertices[i]));
      }
    }

  } else {
    for (size_t i = 0; i < ps->vertices.size(); i++) corner_selected.push_back(true);
  }
  return createFilletInt(ps, corner_selected, this->r, this->fn, this->minang);
}

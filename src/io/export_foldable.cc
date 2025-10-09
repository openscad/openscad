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

#include "src/geometry/PolySet.h"
#include "src/geometry/cgal/cgalutils.h"
#include "src/geometry/PolySetUtils.h"
#include <unordered_map>
#include "src/utils/boost-utils.h"
#include <src/utils/hash.h>
#ifdef ENABLE_CAIRO
#include <cairo.h>
#include <cairo-pdf.h>
#endif

#include "export_foldable.h"
#include "src/geometry/GeometryEvaluator.h"

Vector2d pointrecht(Vector2d x)
{
  Vector2d y;
  y[0] = x[1];
  y[1] = -x[0];
  return y;
}

int point_in_polygon(const std::vector<Vector2d>& pol, const Vector2d& pt)
{
  // polygons are clockwise
  int cuts = 0;
  int n = pol.size();
  for (int i = 0; i < n; i++) {
    Vector2d p1 = pol[i];
    Vector2d p2 = pol[(i + 1) % n];
    if (fabs(p1[1] - p2[1]) > 1e-9) {
      if (pt[1] < p1[1] && pt[1] > p2[1]) {
        double x = p1[0] + (p2[0] - p1[0]) * (pt[1] - p1[1]) / (p2[1] - p1[1]);
        if (x > pt[0]) cuts++;
      }
      if (pt[1] < p2[1] && pt[1] > p1[1]) {
        double x = p1[0] + (p2[0] - p1[0]) * (pt[1] - p1[1]) / (p2[1] - p1[1]);
        if (x > pt[0]) cuts++;
      }
    }
  }
  return cuts & 1;
}
int edge_outwards(std::vector<connS>& con, unsigned int plate, unsigned int face)
{
  int outwards = 0;
  for (unsigned int k = 0; k < con.size(); k++) {
    if (con[k].p2 == plate && con[k].f2 == face) outwards = 1;
  }
  return outwards;
}

void calc_platepoints(plateS& plate, IndexedFace& face, const std::vector<Vector3d>& vertices,
                      const Matrix4d& invmat, const Vector2d& pm, Vector2d px, Vector2d py,
                      plotSettingsS& plot_s)
{
  plate.pt.clear();
  plate.pt_l1.clear();
  plate.pt_l2.clear();

  int n = face.size();
  for (int i = 0; i < n; i++) {
    Vector3d pt = vertices[face[i]];
    Vector4d pt4(pt[0], pt[1], pt[2], 1);
    pt4 = invmat * pt4;
    plate.pt.push_back(px * pt4[0] + py * pt4[1] + pm);
  }
  // nun laschen planen
  for (int i = 0; i < n; i++) {
    py = plate.pt[(i + 1) % n] - plate.pt[i];
    //    double maxl=py.norm();
    double lasche_eff = plot_s.lasche;
    if (lasche_eff > py.norm() / 2.0) lasche_eff = py.norm() / 2.0;
    py.normalize();
    px = pointrecht(py);
    px = -px;
    plate.pt_l1.push_back(plate.pt[i] + (px + py) * lasche_eff);
    py = -py;
    plate.pt_l2.push_back(plate.pt[(i + 1) % n] + (px + py) * lasche_eff);
  }
}

int plot_try(unsigned int refplate, unsigned int destplate, Vector2d px, Vector2d py, Vector2d pm,
             std::vector<plateS>& plate, std::vector<connS>& con, std::vector<Vector3d> vertices,
             std::vector<IndexedFace> faces, std::vector<int>& faceParent, sheetS& sheet,
             unsigned int destedge, plotSettingsS plot_s)
{
  unsigned int i;
  Vector2d p1;
  int success = 1;
  unsigned int n = faces[destplate].size();
  Vector3d totalnorm(0, 0, 0);
  for (unsigned int i = 0; i < n; i++) {
    int i0 = faces[destplate][(i + n - 1) % n];
    int i1 = faces[destplate][(i + 0) % n];
    int i2 = faces[destplate][(i + 1) % n];
    Vector3d dir1 = (vertices[i2] - vertices[i1]).normalized();
    Vector3d dir2 = (vertices[i0] - vertices[i1]).normalized();
    totalnorm += dir1.cross(dir2);
  }
  int i0 = faces[destplate][(destedge + n - 1) % n];
  int i1 = faces[destplate][(destedge + 0) % n];
  int i2 = faces[destplate][(destedge + 1) % n];
  Vector3d pt = vertices[i1];
  Vector3d xdir = (vertices[i2] - pt).normalized();
  Vector3d zdir = (xdir.cross(vertices[i0] - pt)).normalized();
  if (totalnorm.dot(zdir) < 0) zdir = -zdir;  // fix concave edge
  Vector3d ydir = (zdir.cross(xdir)).normalized();
  pt = (vertices[i1] + vertices[i2]) / 2;
  Matrix4d mat;
  mat << xdir[0], ydir[0], zdir[0], pt[0], xdir[1], ydir[1], zdir[1], pt[1], xdir[2], ydir[2], zdir[2],
    pt[2], 0, 0, 0, 1;

  Matrix4d invmat = mat.inverse();

  calc_platepoints(plate[destplate], faces[destplate], vertices, invmat, pm, px, py, plot_s);
  for (unsigned int j = 0; j < faceParent.size(); j++) {
    if (faceParent[j] == (int)destplate)
      calc_platepoints(plate[j], faces[j], vertices, invmat, pm, px, py, plot_s);
  }

  // create complete view of bnd
  plate[destplate].bnd.clear();
  for (unsigned int i = 0; i < n; i++) {
    plate[destplate].bnd.push_back(plate[destplate].pt[i]);
    if (edge_outwards(con, destplate, i)) {
      plate[destplate].bnd.push_back(plate[destplate].pt_l1[i]);
      plate[destplate].bnd.push_back(plate[destplate].pt_l2[i]);
    }
  }
  //
  // check if new points collide with some existing boundaries

  for (unsigned int j = 0; j < plate.size(); j++) {  // kollision mit anderen platten
    if (j == destplate) continue;                    // nicht mit sich selbst
    if (plate[j].done != 1) continue;                // und nicht wenn sie nicht existiert
    for (unsigned int i = 0; i < n; i++) {
      if (point_in_polygon(plate[j].bnd, plate[destplate].pt[i])) success = 0;  // alle neue punkte
      if (j == refplate && i == destedge) {
      }  // joker
      else {
        // jede neue lasche wenn sie auswaerts geht
        // plate destplate, pt i
        //        if(edge_outwards(con, destplate, i) ) {
        // if(point_in_polygon(plate[j].pt,plate[destplate].pt_l1[i])) success=0; // alle neue laschen
        // if(point_in_polygon(plate[j].pt,plate[destplate].pt_l2[i])) success=0;
        //	}
      }
    }
  }
  //
  // check of any old points collide with  the new boundary
  for (unsigned int j = 0; j < plate.size(); j++) {  // kollision mit anderen platten
    if (j == destplate) continue;                    // nicht mit sich selbst
    if (plate[j].done != 1) continue;                // und nicht wenn sie nicht existiert
    unsigned int n = plate[j].pt.size();
    for (unsigned int i = 0; i < n; i++) {
      if (j == refplate) continue;
      for (unsigned int i = 0; i < plate[j].pt.size(); i++) {
        if (point_in_polygon(plate[destplate].pt, plate[j].pt[i])) success = 0;
      }
    }
  }
  // TODO page3 und 4 auch zusammen fassen
  for (i = 0; i < n; i++) {
    lineS line;
    line.p1 = plate[destplate].pt[i];
    line.p2 = plate[destplate].pt[(i + 1) % n];
    line.dashed = 0;
    sheet.lines.push_back(line);
  }
  for (unsigned int j = 0; j < faceParent.size(); j++) {
    if (faceParent[j] == (int)destplate) {
      int n1 = plate[j].pt.size();
      lineS line;
      for (int i = 0; i < n1; i++) {
        line.p1 = plate[j].pt[i];
        line.p2 = plate[j].pt[(i + 1) % n1];
        line.dashed = 0;
        sheet.lines.push_back(line);
      }
    }
  }

  for (i = 0; i < sheet.lines.size(); i++) {
    for (int j = 0; j < 2; j++) {
      if (i == 0 || sheet.lines[i].p1[j] < sheet.min[j]) sheet.min[j] = sheet.lines[i].p1[j];
      if (i == 0 || sheet.lines[i].p1[j] > sheet.max[j]) sheet.max[j] = sheet.lines[i].p1[j];
      if (i == 0 || sheet.lines[i].p2[j] < sheet.min[j]) sheet.min[j] = sheet.lines[i].p2[j];
      if (i == 0 || sheet.lines[i].p2[j] > sheet.max[j]) sheet.max[j] = sheet.lines[i].p2[j];
    }
  }
  if (plot_s.paperwidth - (sheet.max[0] - sheet.min[0]) < 2 * plot_s.rand) success = 0;
  if (plot_s.paperheight - (sheet.max[1] - sheet.min[1]) < 2 * plot_s.rand) success = 0;
  return success;
}

class EdgeDbStub
{
public:
  int ind1, ind2, ind3;
  int operator==(const EdgeDbStub ref)
  {
    if (this->ind1 == ref.ind1 && this->ind2 == ref.ind2) return 1;
    return 0;
  }
};

unsigned int hash_value(const EdgeDbStub& r)
{
  unsigned int i;
  i = r.ind1 | (r.ind2 << 16);
  return i;
}

int operator==(const EdgeDbStub& t1, const EdgeDbStub& t2)
{
  if (t1.ind1 == t2.ind1 && t1.ind2 == t2.ind2) return 1;
  return 0;
}
//

typedef std::vector<double> doubleList;

int cut_line_line(Vector2d p1, Vector2d n1, Vector2d p2, Vector2d n2, Vector2d& res)
{
  double det = n2[1] * n1[0] - n2[0] * n1[1];
  if (fabs(det) < 1e-6) return 1;

  double dx = p2[0] - p1[0];
  double dy = p2[1] - p1[1];

  res[0] = (dx * n2[1] - dy * n2[0]) / det;
  res[1] = (dx * n1[1] - dy * n1[0]) / det;

  return 0;
}

doubleList create_radExtent(sheetS& sheet)
{
  int i;
  doubleList extent;

  for (unsigned i = 0; i < sheet.lines.size(); i++) {
    for (int j = 0; j < 2; j++) {
      if (i == 0 || sheet.lines[i].p1[j] < sheet.min[j]) sheet.min[j] = sheet.lines[i].p1[j];
      if (i == 0 || sheet.lines[i].p1[j] > sheet.max[j]) sheet.max[j] = sheet.lines[i].p1[j];
      if (i == 0 || sheet.lines[i].p2[j] < sheet.min[j]) sheet.min[j] = sheet.lines[i].p2[j];
      if (i == 0 || sheet.lines[i].p2[j] > sheet.max[j]) sheet.max[j] = sheet.lines[i].p2[j];
    }
  }

  Vector2d mid = Vector2d((sheet.min[0] + sheet.max[0]) / 2.0, (sheet.min[1] + sheet.max[1]) / 2.0);
  Vector2d res;
  for (i = 0; i < 12; i++) {
    double ang = i * G_PI / 6.0;
    Vector2d dir(+cos(ang), sin(ang));
    // find farthest cutpoint in this direction
    double ext = 0;
    for (unsigned int j = 0; j < sheet.lines.size(); j++) {
      if (cut_line_line(mid, dir, sheet.lines[j].p1, sheet.lines[j].p2 - sheet.lines[j].p1, res))
        continue;
      if (res[1] < 0 || res[1] > 1) continue;  // no hit
      if (res[0] < 0) continue;                // not behind
      if (res[0] > ext) ext = res[0];
    }
    extent.push_back(ext);
    //    printf("%g %g\n",i*30.0, ext);
  }
  return extent;
}
std::vector<sheetS> sheets_combine(std::vector<sheetS> sheets, const plotSettingsS& plot_s)
{
  std::vector<doubleList> sheet_extent;
  for (auto& sheet : sheets) {
    sheet_extent.push_back(create_radExtent(sheet));
  }
  //  const char *debugstr = getenv("DEBUG"); // TODO geht mit DEBUGX anders ?
  //  int debug=0;
  //  if(debugstr != NULL) {
  //	  sscanf(debugstr,"%d",&debug);
  //  }
  //  printf("sheets_combine src sheets is %ld\n",sheets.size());
  std::vector<sheetS> combined;
  std::vector<doubleList> combined_extent;
  for (unsigned int i = 0; i < sheets.size(); i++) {
    //    printf(".\n");
    bool success = false;
    // try to combine new with all existing sheets
    for (unsigned int j = 0; !success && j < combined.size(); j++) {
      for (int k = 0; !success && k < 12; k++) {  // all 12 angles
                                                  // calculate relative distplacement
        double disp = combined_extent[j][k] + sheet_extent[i][(k + 6) % 12] + 5;
        Vector2d dispv = Vector2d(disp * cos(k * G_PI / 6.0), disp * sin(k * G_PI / 6.0)) +
                         (combined[j].min + combined[j].max) / 2.0 -
                         (sheets[i].min + sheets[i].max) / 2.0;
        // create combined
        sheetS Union = combined[j];
        int orglen = Union.lines.size();
        sheetS& ref = sheets[i];
        for (unsigned int l = 0; l < ref.lines.size(); l++) {
          lineS newL = ref.lines[l];
          newL.p1 += dispv;
          newL.p2 += dispv;
          Union.lines.push_back(newL);
        }
        for (unsigned int l = 0; l < ref.label.size(); l++) {
          labelS newL = ref.label[l];
          newL.pt += dispv;
          Union.label.push_back(newL);
        }
        doubleList Union_extent = create_radExtent(Union);
        if (plot_s.paperwidth - (Union.max[0] - Union.min[0]) < 2 * plot_s.rand) continue;
        if (plot_s.paperheight - (Union.max[1] - Union.min[1]) < 2 * plot_s.rand) continue;
        Vector2d res;
        bool collision = false;
        for (int l = 0; !collision && l < orglen; l++) {
          lineS& l1 = Union.lines[l];
          for (unsigned int m = orglen; !collision && m < Union.lines.size(); m++) {
            lineS l2 = Union.lines[m];
            if (cut_line_line(l1.p1, l1.p2 - l1.p1, l2.p1, l2.p2 - l2.p1, res)) continue;
            if (res[0] > 0 && res[0] < 1 && res[1] > 0 && res[1] < 1) collision = true;
          }
        }
        if (collision == true) continue;
        success = true;
        combined[j] = Union;
        combined_extent[j] = create_radExtent(Union);
        //	printf("%d fits next to %d with angle %d\n",i,j,k*30);
      }
    }
    if (!success) {
      combined.push_back(sheets[i]);
      combined_extent.push_back(sheet_extent[i]);
      //      printf("new page %ld for %d\n",combined.size(),i);
    }
  }
  return combined;
}

void debug_conn(std::vector<connS>& con, std::vector<IndexedFace>& faces,
                const std::vector<Vector3d>& vertices)
{
  for (unsigned int i = 0; i < con.size(); i++) {
    auto& a = con[i];
    int n = faces[a.p1].size();
    double da = (vertices[faces[a.p1][a.f1]] - vertices[faces[a.p1][(a.f1 + 1) % n]]).norm();
    printf("Con %d:done=%d length=%g  %d:%d ->%d:%d \n", i, con[i].done, da, con[i].p1, con[i].f1,
           con[i].p2, con[i].f2);
  }
}
std::vector<sheetS> fold_3d(std::shared_ptr<const PolySet> ps, const plotSettingsS& plot_s)
{
  std::vector<Vector4d> normals = calcTriangleNormals(ps->vertices, ps->indices);
  std::vector<Vector4d> newNormals;
  std::vector<int> faceParents;
  std::vector<IndexedFace> faces =
    mergeTriangles(ps->indices, normals, newNormals, faceParents, ps->vertices);
  unsigned int i, j, k;
  int glue, num = 0, cont, success, drawn, other;
  int facesdone = 0, facestodo;
  //  for(int i=0;i<faces.size();i++)
  //  {
  //	printf("%d %g/%g/%g/%g par=%d\n",i, newNormals[i][0], newNormals[i][1], newNormals[i][2],
  // newNormals[i][3], faceParents[i]);
  //  }

  //
  // create edge database

  //  EdgeDbStub stub;
  //  std::unordered_map<EdgeDbStub, int, boost::hash<EdgeDbStub> > edge_db;
  // std::unordered_map<EdgeDbStub, int> edge_db;
  //

  for (unsigned i = 0; i < faces.size(); i++) {
    IndexedFace& face = faces[i];
    unsigned int n = face.size();
    for (j = 0; j < n; j++) {
      //      stub.ind1=face[j];
      //      stub.ind2=face[(j+1)%n];
      //      edge_db[stub]=i;
    }
  }
  std::vector<connS> con;
  connS cx;
  cx.done = 0;

  for (unsigned i = 0; i < faces.size(); i++) {
    IndexedFace& face = faces[i];
    unsigned int n = face.size();
    for (j = 0; j < n; j++) {
      int i1 = face[j];
      int i2 = face[(j + 1) % n];
      if (i2 < i1) continue;

      // TODO stupid workaround
      int oppface = -1;
      int opppos = 0;
      for (unsigned int k = 0; oppface == -1 && k < faces.size(); k++) {
        IndexedFace& face1 = faces[k];
        int n1 = face1.size();
        for (int l = 0; oppface == -1 && l < n1; l++) {
          int I1 = face1[l];
          int I2 = face1[(l + 1) % n1];
          if (i1 == I2 && i2 == I1) {
            oppface = k;
            opppos = l;
          }
        }
      }
      //      printf("%d/%d -> %d/%d\n",i,j,oppface,opppos);
      cx.p1 = i;
      cx.f1 = j;
      cx.p2 = oppface;
      cx.f2 = opppos;
      con.push_back(cx);

      // i,j, was ist gegnueber
    }
  }

  std::sort(con.begin(), con.end(), [ps, faces](const connS& a, const connS& b) {
    int na = faces[a.p1].size();
    int nb = faces[b.p1].size();
    double da = (ps->vertices[faces[a.p1][a.f1]] - ps->vertices[faces[a.p1][(a.f1 + 1) % na]]).norm();
    double db = (ps->vertices[faces[b.p1][b.f1]] - ps->vertices[faces[b.p1][(b.f1 + 1) % nb]]).norm();
    return (da > db) ? 1 : 0;
  });

  //  std::vector<lineS> lines,linesorg; // final postscript lines

  Vector2d px, py, p1(0, 0), p2, pm;
  std::vector<plateS> plate;  // faces in final placement
                              //  int polybesttouch;

  /*

    bestend=reorder_edges(eder);
  */

  facestodo = faces.size();

  plateS newplate;
  newplate.done = 0;

  for (i = 0; i < faces.size(); i++) plate.push_back(newplate);

  //  const char *debugstr = getenv("DEBUG");
  //  int debug=0;
  //  if(debugstr != NULL) {
  //	  sscanf(debugstr,"%d",&debug);
  //  }

  std::vector<sheetS> sheets;
  int pagenum = 0;
  while (facesdone < facestodo) {
    //    printf("Restart facesdone=%d\n",facesdone);
    // ein blatt designen
    //    polybesttouch=0;
    cont = 1;
    sheetS sheet, sheetorg;
    while (cont == 1) {
      cont = 0;
      drawn = 0;
      if (sheet.lines.size() == 0) {
        for (unsigned i = 0; i < faces.size() && drawn == 0; i++) {
          if (plate[i].done == 0 && faceParents[i] == -1) {
            success = 0;
            sheetorg = sheet;
            pm[0] = 0;
            pm[1] = 0;
            px = pm;
            px[0] = 1;
            py = pointrecht(px);

            success = plot_try(-1, i, px, py, pm, plate, con, ps->vertices, faces, faceParents, sheet, 0,
                               plot_s);

            if (success == 1) {
              //              printf("Successfully placed plate %d px=%g/%g, py=%g/%g\n",i,px[0], px[1],
              //              py[0], py[1]);
              plate[i].done = 1;
              facesdone++;
              for (unsigned int j = 0; j < faceParents.size(); j++) {
                if (faceParents[j] == (int)i) {
                  plate[j].done = 1;
                  facesdone++;
                }
              }
              cont = 1;
              drawn = 1;
            } else {
              sheet = sheetorg;
            }
          }
        }
      } else {
        for (i = 0; i < con.size() && drawn == 0; i++) {
          if (con[i].done) continue;
          int p1 = -1, p2 = -1, f1 = -1, f2 = -1;
          if (con[i].p1 >= faces.size()) continue;
          if (con[i].p2 >= faces.size()) continue;
          if (plate[con[i].p1].done == 1 && plate[con[i].p2].done == 0) {
            p1 = con[i].p1;
            f1 = con[i].f1;
            p2 = con[i].p2;
            f2 = con[i].f2;
          }
          if (plate[con[i].p1].done == 0 && plate[con[i].p2].done == 1) {
            p2 = con[i].p1;
            f2 = con[i].f1;
            p1 = con[i].p2;
            f1 = con[i].f2;
          }
          if (p1 != -1 && faceParents[p2] == -1) {
            // draw p1:f1 -> p2:f2
            int n1 = plate[p1].pt.size();
            Vector2d pt1 = plate[p1].pt[f1];
            Vector2d pt2 = plate[p1].pt[(f1 + 1) % n1];
            Vector2d px = (pt1 - pt2).normalized();
            Vector2d py = pointrecht(px);
            Vector2d pm = (pt1 + pt2) / 2;
            sheetorg = sheet;

            success = plot_try(p1, p2, px, py, pm, plate, con, ps->vertices, faces, faceParents, sheet,
                               f2, plot_s);
            //                if(b == bestend)
            //                {
            //                  if(polybesttouch == 0) polybesttouch=1; else success=0;
            //                }
            if (success == 1) {
              //              printf("Successfully placed plate %d px=%g/%g, py=%g/%g\n",p2,px[0], px[1],
              //              py[0], py[1]);
              plate[p2].done = 1;
              facesdone++;
              for (unsigned int j = 0; j < faceParents.size(); j++) {
                if (faceParents[j] == p2) {
                  plate[j].done = 1;
                  facesdone++;
                }
              }
              drawn = 1;
              con[i].done = 1;
              cont = 1;
            } else {
              sheet = sheetorg;
            }
          }
        }
      }
    }
    //----------------------------------- Alles fertigstellen

    // nachtraegliche laschenzeichnung
    if (sheet.lines.size() == 0) {
      printf("Was not able to fit something onto the page!\n");
      return sheets;
    }
    labelS lnew;
    for (i = 0; i < plate.size(); i++)  // plates
    {
      if (plate[i].done == 1) {
        unsigned int n = plate[i].pt.size();
        Vector2d mean(0, 0);
        for (j = 0; j < n; j++)  // pts
        {
          mean += plate[i].pt[j];
          glue = 0;
          for (k = 0; k < con.size(); k++)  // connections
          {
            if (con[k].p1 == i && con[k].f1 == j && con[k].done == 0) {
              num = k;
              glue = 1;
              other = con[k].p2;
            }
            if (con[k].p2 == i && con[k].f2 == j && con[k].done == 0) {
              num = k;
              glue = -1;
              other = con[k].p1;
            }
          }
          if (glue != 0) {
            p1 = plate[i].pt_l1[j];
            p2 = plate[i].pt_l2[j];

            if (glue < 0) {  // nur aussenkannte
              lineS line;
              line.dashed = 0;
              line.p1 = plate[i].pt[j];
              line.p2 = p1;
              sheet.lines.push_back(line);
              line.p1 = p1;
              line.p2 = p2;
              sheet.lines.push_back(line);
              line.p1 = p2;
              line.p2 = plate[i].pt[(j + 1) % n];
              sheet.lines.push_back(line);
              line.p1 = plate[i].pt[(j + 1) % n];
              line.p2 = plate[i].pt[j];
              sheet.lines.push_back(line);
            }

            if (plate[other].done != 1)  // if the connection is not to the same page
            {
              p1 = (plate[i].pt[j] + plate[i].pt[(j + 1) % n]) * 0.5;

              py = plate[i].pt[(j + 1) % n] - plate[i].pt[j];  // entlang der kante
              double lasche_eff = plot_s.lasche;
              if (lasche_eff > py.norm() / 2) lasche_eff = py.norm() / 2;
              py.normalize();
              px = pointrecht(py);

              p1 = p1 + px * lasche_eff * (0.5 - 0.27);
              p1 = p1 + py * lasche_eff * -0.35;
              lnew.pt = p1;
              snprintf(lnew.text, sizeof(lnew.text), "%d", num);
              lnew.rot = atan2(py[1], py[0]) * 180.0 / 3.1415;
              lnew.size = lasche_eff;
              sheet.label.push_back(lnew);
            }
          }
        }
        //        lnew.pt=p1; // for DEBUG only
        //        sprintf(lnew.text,"%d",i);
        //        lnew.pt = mean / n;
        //        lnew.rot=0;
        //        sheet.label.push_back(lnew);
      }
    }
    lnew.pt = p1;  // Page number
                   //    sprintf(lnew.text,"%s/%d","a.ps", pagenum+1); // TODO fix
                   //    lnew.pt[0]=10;
                   //    lnew.pt[1]=10;
                   //    sheet.label.push_back(lnew);
    for (i = 0; i < faces.size(); i++) {
      if (plate[i].done == 1) {
        plate[i].done = 2;
      }
    }
    for (unsigned int i = 0; i < sheet.lines.size() - 1; i++) {
      auto& line1 = sheet.lines[i];
      for (unsigned int j = i + 1; j < sheet.lines.size(); j++) {
        auto& line2 = sheet.lines[j];
        if ((line1.p1 - line2.p2).norm() < 1e-3 && (line1.p2 - line2.p1).norm() < 1e-3) {
          line1.dashed = 1;
          sheet.lines.erase(sheet.lines.begin() + j);
          break;
        }
      }
    }
    sheets.push_back(sheet);
    pagenum++;
  }
  return sheets_combine(sheets, plot_s);
}

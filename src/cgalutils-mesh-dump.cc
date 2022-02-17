#include "cgalutils.h"

#include "CGALHybridPolyhedron.h"
#include "Polygon2d.h"

namespace CGALUtils {

template <typename TriangleMesh, typename OutStream>
void dumpMesh(const TriangleMesh &tm, size_t dimension, size_t convexity, OutStream &out, const std::string &indent, size_t currindent)
{
  typedef boost::graph_traits<TriangleMesh> GT;
  typedef typename GT::vertex_descriptor vertex_descriptor;

  if (dimension != 2 && dimension != 3) throw 0;

  auto printIndents = [&]() {
    for (int i = 0; i < currindent; ++i) {
      out << indent;
    }
  };

  printIndents();
  out << (dimension == 2 ? "polygon" : "polyhedron") << "(\n";
  currindent++;

  printIndents();
  out << "points = [\n";
  currindent++;

  std::unordered_map<vertex_descriptor, size_t> vertexIndices;
  auto nextVertexIndex = 0;

  for (auto &v : tm.vertices()) {
    if (tm.is_removed(v)) continue;

    auto vertexIndex = vertexIndices[v] = nextVertexIndex++;

    auto& p = tm.point(v);
    double x = CGAL::to_double(p.x());
    double y = CGAL::to_double(p.y());
    double z = CGAL::to_double(p.z());

    printIndents();
    out << "[" << x << ", " << y;
    if (dimension == 3) out << ", " << z;
    out << "], \t// " << vertexIndex << "\n";
  }
  currindent--;
  printIndents();
  out << "],\n";

  printIndents();
  out << (dimension == 2 ? "paths" : "faces") << " = [\n";
  currindent++;
  for (auto &f : tm.faces()) {
    if (tm.is_removed(f)) continue;

    auto first = true;
    printIndents();
    out << "  [";

    CGAL::Vertex_around_face_iterator<TriangleMesh> vit, vend;
    for (boost::tie(vit, vend) = vertices_around_face(tm.halfedge(f), tm); vit != vend; ++vit) {
      auto v = *vit;

      auto it = vertexIndices.find(v);
      if (it == vertexIndices.end()) {
        LOG(message_group::Error, Location::NONE, "", "Invalid point in mesh");
        continue;
      }

      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << it->second;
    }
    out << "],\n";
  }
  currindent--;
  printIndents();
  out << "],\n";
  
  printIndents();
  out << "convexity = " << convexity << "\n";

  currindent--;
  printIndents();
  out << ");\n";
}

template void dumpMesh(const CGAL_HybridMesh &tm, size_t dimension, size_t convexity, std::ostringstream &o, const std::string &indent, size_t currindent);

} // namespace CGALUtils

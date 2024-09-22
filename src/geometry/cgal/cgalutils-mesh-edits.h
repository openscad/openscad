// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "utils/printutils.h"

namespace CGALUtils {

namespace PMP = CGAL::Polygon_mesh_processing;

/*! Buffer of changes to be applied to a triangle mesh. */
template <typename TriangleMesh>
class TriangleMeshEdits
{

private:
  using GT = boost::graph_traits<TriangleMesh>;
  using face_descriptor = typename GT::face_descriptor;
  using halfedge_descriptor = typename GT::halfedge_descriptor;
  using edge_descriptor = typename GT::edge_descriptor;
  using vertex_descriptor = typename GT::vertex_descriptor;

  std::unordered_set<face_descriptor> facesToRemove;
  std::unordered_set<vertex_descriptor> verticesToRemove;
  std::vector<std::vector<vertex_descriptor>> facesToAdd;
  std::unordered_map<vertex_descriptor, vertex_descriptor> vertexReplacements;

public:

  bool isEmpty() {
    return facesToRemove.empty() &&
           verticesToRemove.empty() &&
           facesToAdd.empty() &&
           vertexReplacements.empty();
  }

  void removeFace(const face_descriptor& f) {
    facesToRemove.insert(f);
  }

  void removeVertex(const vertex_descriptor& v) {
    verticesToRemove.insert(v);
  }

  void addFace(const std::vector<vertex_descriptor>& vertices) {
    facesToAdd.push_back(vertices);
  }

  void replaceVertex(const vertex_descriptor& original, const vertex_descriptor& replacement) {
    vertexReplacements[original] = replacement;
  }

  static bool findCollapsibleVertices(
    const std::vector<vertex_descriptor>& path,
    const TriangleMesh& tm,
    const std::function<void(size_t, vertex_descriptor)>& sinkFn) {
    if (path.size() <= 3) {
      return false;
    }

    const auto *p1 = &tm.point(path[0]);
    const auto *p2 = &tm.point(path[1]);
    const auto *p3 = &tm.point(path[2]);

    for (size_t i = 0, n = path.size(); i < n; i++) {
      if (CGAL::are_ordered_along_line(*p1, *p2, *p3)) {
        // p2 (at index i + 1) can be removed.
        auto ii = (i + 1) % n;
        sinkFn(ii, path[ii]);
      }
      p1 = p2;
      p2 = p3;
      p3 = &tm.point(path[(i + 3) % n]);
    }

    return true;
  }

  /*! Mutating in place is tricky, to say the least, so this creates a new mesh
   * and overwrites the original to it at the end for now. */
  bool apply(TriangleMesh& src) const
  {
    TriangleMesh copy;
    auto wasValid = CGAL::is_valid_polygon_mesh(src);
    auto wasClosed = CGAL::is_closed(src);

    auto edgesAdded = 0;
    for (auto& vs : facesToAdd) edgesAdded += vs.size();

    auto projectedVertexCount = src.number_of_vertices() - verticesToRemove.size();
    auto projectedHalfedgeCount = src.number_of_halfedges() + edgesAdded * 2; // This is crude
    auto projectedFaceCount = src.number_of_faces() - facesToRemove.size() + facesToAdd.size();
    copy.reserve(copy.number_of_vertices() + projectedVertexCount,
                 copy.number_of_halfedges() + projectedHalfedgeCount,
                 copy.number_of_faces() + projectedFaceCount);

    // TODO(ochafik): Speed up with a lookup vector : std::vector<vertex_descriptor> vertexMap(src.number_of_vertices());
    std::unordered_map<vertex_descriptor, vertex_descriptor> vertexMap;
    vertexMap.reserve(projectedVertexCount);

    auto getDestinationVertex = [&](auto srcVertex) {
        auto repIt = vertexReplacements.find(srcVertex);
        if (repIt != vertexReplacements.end()) {
          srcVertex = repIt->second;
        }
        auto it = vertexMap.find(srcVertex);
        if (it == vertexMap.end()) {
          auto v = copy.add_vertex(src.point(srcVertex));
          vertexMap[srcVertex] = v;
          return v;
        }
        return it->second;
      };

    std::vector<vertex_descriptor> polygon;

    auto addFace = [&](auto& polygon) {
        auto face = copy.add_face(polygon);
        if (face.is_valid()) {
          if (polygon.size() > 3) {
            PMP::triangulate_face(face, copy);
          }
          return true;
        } else {
          LOG(message_group::Warning, "Failed to add face with %1$lu vertices!", polygon.size());
          return false;
        }
      };
    auto copyFace = [&](auto& f) {
        polygon.clear();

        CGAL::Vertex_around_face_iterator<TriangleMesh> vit, vend;
        for (boost::tie(vit, vend) = vertices_around_face(src.halfedge(f), src); vit != vend; ++vit) {
          auto v = *vit;
          if (verticesToRemove.find(v) != verticesToRemove.end()) {
            continue;
          }
          polygon.push_back(getDestinationVertex(v));
        }
        if (polygon.size() < 3) {
          LOG(message_group::Warning, "Attempted to remove too many vertices around this copied face, remesh aborted!");
          return false;
        }

        return addFace(polygon);
      };

    for (auto f : src.faces()) {
      if (src.is_removed(f)) {
        continue;
      }
      if (facesToRemove.find(f) != facesToRemove.end()) {
        continue;
      }
      if (!copyFace(f)) {
        return false;
      }
    }

    for (auto& originalPolygon : facesToAdd) {
      polygon.clear();

      for (auto v : originalPolygon) {
        if (verticesToRemove.find(v) != verticesToRemove.end()) {
          continue;
        }
        polygon.push_back(getDestinationVertex(v));
      }
      if (polygon.size() < 3) {
        LOG(message_group::Warning, "Attempted to remove too many vertices around this added face, remesh aborted!");
        return false;
      }
      if (!addFace(polygon)) {
        return false;
      }
    }

    if (wasValid && !CGAL::is_valid_polygon_mesh(copy)) {
      LOG(message_group::Warning, "Remeshing output isn't valid");
      return false;
    }
    if (wasClosed && !CGAL::is_closed(copy)) {
      LOG(message_group::Warning, "Remeshing output isn't closed");
      return false;
    }

    src = copy;

    return true;
  }
};

} // namespace CGALUtils

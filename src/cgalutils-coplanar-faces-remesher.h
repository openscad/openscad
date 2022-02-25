// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include <iterator>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Kernel/global_functions.h>
#include <CGAL/Kernel/function_objects.h>

#include "cgalutils-mesh-edits.h"

namespace CGALUtils {

/*! Helper that remeshes coplanar faces.
 *
 * This is useful for the results of corefinement operations, which result in
 * suboptimal meshes which growth compounds over several operations and makes
 * some models exponentially slow.
 *
 * Two faces with the same patch id *MUST* be coplanar, but need not be continuous
 * (technically the same patch id could result in different patches, e.g. if
 * during corefinement the same original face got split in disjoint sets of
 * descendants).
 */
template <typename TriangleMesh>
struct CoplanarFacesRemesher {
  typedef boost::graph_traits<TriangleMesh> GT;
  typedef typename GT::face_descriptor face_descriptor;
  typedef typename GT::halfedge_descriptor halfedge_descriptor;
  typedef typename GT::edge_descriptor edge_descriptor;
  typedef typename GT::vertex_descriptor vertex_descriptor;

  typedef std::function<bool (const face_descriptor&)> face_predicate;

  TriangleMesh& tm;

public:

  CoplanarFacesRemesher(TriangleMesh& mesh) : tm(mesh) {}

  template <typename PatchId>
  void remesh(
    const std::unordered_map<face_descriptor, PatchId>& faceToCoplanarPatchId,
    const std::unordered_set<PatchId>& patchesToRemesh)
  {
    if (patchesToRemesh.empty()) return;

    auto facesBefore = tm.number_of_faces();
    auto verbose = Feature::ExperimentalFastCsgDebug.is_enabled();

    // auto patchToPolyhedronStr = [&](auto &patchFaces) {
    //   std::vector<vertex_descriptor> vertices; 
    //   std::map<vertex_descriptor, size_t> vertexIndex;

    //   std::ostringstream verticesOut;
    //   std::ostringstream facesOut;

    //   facesOut << "[\n";
    //   for (auto& face : patchFaces) {
    //     CGAL::Halfedge_around_face_iterator<TriangleMesh> heIt, heEnd;
    //     facesOut << "  [";
    //     auto first = true;
    //     for (boost::tie(heIt, heEnd) = halfedges_around_face(tm.halfedge(face), tm); heIt != heEnd; ++heIt) {
    //       auto he = *heIt;
    //       auto v = tm.source(he);
    //       auto it = vertexIndex.find(v);
    //       size_t idx;
    //       if (it == vertexIndex.end()) {
    //         idx = vertices.size();
    //         vertices.push_back(v);
    //         vertexIndex[v] = idx;
    //       } else {
    //         idx = it->second;
    //       }
    //       if (first) first = false;
    //       else facesOut << ", ";
    //       facesOut << idx;
    //     }
    //     facesOut << "],\n";
    //   }
    //   facesOut << "]";

    //   verticesOut << "[\n";
    //   for (auto v : vertices) {
    //     auto p = tm.point(v);
    //     verticesOut << "[" << CGAL::to_double(p.x()) << ", " << CGAL::to_double(p.y()) << ", " << CGAL::to_double(p.z()) << "],\n";
    //   }
    //   verticesOut << "]";

    //   std::ostringstream out;
    //   out << "polyhedron(" << verticesOut.str().c_str() << ", " << facesOut.str().c_str() << ");";
    //   return out.str();
    // };

    // auto patchBorderToPolyhedronStr = [&](auto &borderPathVertices) {
    //   std::vector<vertex_descriptor> vertices; 
    //   std::map<vertex_descriptor, size_t> vertexIndex;

    //   std::ostringstream verticesOut;
    //   std::ostringstream facesOut;

    //   facesOut << "[[";
    //   auto first = true;
    //   for (auto& v : borderPathVertices) {
    //     auto it = vertexIndex.find(v);
    //     size_t idx;
    //     if (it == vertexIndex.end()) {
    //       idx = vertices.size();
    //       vertices.push_back(v);
    //       vertexIndex[v] = idx;
    //     } else {
    //       idx = it->second;
    //     }
    //     if (first) first = false;
    //     else facesOut << ", ";
    //     facesOut << idx;
    //   }
    //   facesOut << "]]\n";

    //   verticesOut << "[\n";
    //   for (auto v : vertices) {
    //     auto p = tm.point(v);
    //     verticesOut << "[" << CGAL::to_double(p.x()) << ", " << CGAL::to_double(p.y()) << ", " << CGAL::to_double(p.z()) << "],\n";
    //   }
    //   verticesOut << "]";

    //   std::ostringstream out;
    //   out << "polyhedron(" << verticesOut.str().c_str() << ", " << facesOut.str().c_str() << ");";
    //   return out.str();
    // };

    try {
      TriangleMeshEdits<TriangleMesh> edits;

      std::unordered_set<face_descriptor> facesProcessed;

      std::set<face_descriptor> loopLocalPatchFaces;
      std::map<PatchId, bool> loopLocalIsPatchIdMap;
      std::vector<halfedge_descriptor> loopLocalBorderEdges;
      std::vector<halfedge_descriptor> loopLocalBorderPath;
      std::vector<vertex_descriptor> loopLocalBorderPathVertices;
      std::unordered_set<halfedge_descriptor> loopLocalBorderPathEdges;

      // std::unordered_map<PatchId, std::string> allPatchPolyStrings;
      // std::unordered_map<PatchId, std::string> allPatchReplacementsPolyStrings;

      for (auto face : tm.faces()) {
        if (tm.is_removed(face)) {
          continue;
        }
        auto idIt = faceToCoplanarPatchId.find(face);
        if (idIt == faceToCoplanarPatchId.end()) {
          continue;
        }
        const auto id = idIt->second;

        if (patchesToRemesh.find(id) == patchesToRemesh.end()) {
          continue;
        }
        if (facesProcessed.find(face) != facesProcessed.end()) {
          continue;
        }

        loopLocalIsPatchIdMap.clear();
        auto& isPatchIdMap = loopLocalIsPatchIdMap;
        isPatchIdMap[id] = true;

        loopLocalPatchFaces.clear();
        auto& patchFaces = loopLocalPatchFaces;
        patchFaces.insert(face);

        auto isHalfedgeOnBorder = [&](auto& he) {
            auto neighbourFace = tm.face(tm.opposite(he));
            if (!neighbourFace.is_valid()) {
              return true;
            }

            auto neighbourIdIt = faceToCoplanarPatchId.find(neighbourFace);
            if (neighbourIdIt == faceToCoplanarPatchId.end()) {
              std::cerr << "Unknown face, weird!";
              return true;
            }
            auto neighbourId = neighbourIdIt->second;

            auto isPatchIdIt = isPatchIdMap.find(neighbourId);
            if (isPatchIdIt != isPatchIdMap.end()) {
              auto isPatchId = isPatchIdIt->second;
              return !isPatchId;
            }

            auto isCoplanar = isEdgeBetweenCoplanarFaces(he);
            isPatchIdMap[neighbourId] = isCoplanar;
            return !isCoplanar;
          };

        floodFillPatch(patchFaces, isHalfedgeOnBorder);

        // allPatchPolyStrings[id] = patchToPolyhedronStr(patchFaces);

        for (auto &face : patchFaces) {
          facesProcessed.insert(face);
        }

        if (patchFaces.size() < 2) {
          continue;
        }

        auto isFaceOnPatch = [&](auto& f) {
            return patchFaces.find(f) != patchFaces.end();
          };

        loopLocalBorderEdges.clear();
        auto &borderEdges = loopLocalBorderEdges;

        for (auto& face : patchFaces) {
          CGAL::Halfedge_around_face_iterator<TriangleMesh> heIt, heEnd;
          for (boost::tie(heIt, heEnd) = halfedges_around_face(tm.halfedge(face), tm); heIt != heEnd; ++heIt) {
            auto he = *heIt;
            if (isHalfedgeOnBorder(he)) {
              borderEdges.push_back(he);
            }
          }
        }

        if (borderEdges.empty()) {
          std::cerr << "Failed to find a border edge for patch " << id << "!\n";
          continue;
        }
        halfedge_descriptor borderEdge = *borderEdges.begin();

        loopLocalBorderPath.clear();
        auto& borderPath = loopLocalBorderPath;

        if (!walkAroundPatch(borderEdge, isFaceOnPatch, borderPath)) {
          LOG(message_group::Error, Location::NONE, "",
              "[fast-csg-remesh] Failed to collect path around patch faces, invalid mesh!");
          return;
        }

        if (borderPath.size() <= 3) {
          continue;
        }

        loopLocalBorderPathVertices.clear();
        auto& borderPathVertices = loopLocalBorderPathVertices;

        loopLocalBorderPathEdges.clear();
        auto &borderPathEdges = loopLocalBorderPathEdges;

        for (auto he : borderPath) {
          borderPathEdges.insert(he);
          borderPathVertices.push_back(tm.target(he));
        }

        auto hasHoles = false;
        for (auto he : borderEdges) {
          if (borderPathEdges.find(he) == borderPathEdges.end()) {
            // Found a border halfedge that's not in the border path we've walked around the patch.
            // This means the patch is holed / has more than one border: abort!!
            hasHoles = true;
            break;
          }
        }
        if (hasHoles) {
          if (verbose) {
            LOG(message_group::None, Location::NONE, "",
              "[fast-csg-remesh] Skipping remeshing of patch with %1$lu faces as it seems to have holes.", borderPathEdges.size());
          }
          continue;
        }
        auto lengthBefore = borderPathVertices.size();
        auto collapsed = TriangleMeshEdits<TriangleMesh>::collapsePathWithConsecutiveCollinearEdges(borderPathVertices, tm);

        auto lengthAfter = borderPathVertices.size();
        // if (collapsed && verbose) {
        //   std::cerr << "Collapsed path around patch " << id << " (" << patchFaces.size() << " faces) from " << lengthBefore << " to " << lengthAfter << " vertices\n";
        // }

        // allPatchReplacementsPolyStrings[id] = patchBorderToPolyhedronStr(borderPathVertices);

        // Cover the patch with a polygon. It will be triangulated when the
        // edits are applied.
        
        // edits.replaceFaces(patchFaces, borderPathVertices);

        for (auto& face : patchFaces) edits.removeFace(face);
        edits.addFace(borderPathVertices);
          
      }

      // {
      //   std::ofstream fout("patches.scad");
      //   fout << "before=true;\npatchIndex=-1;\n";
      //   size_t patchIndex = 0;
      //   for (auto &p : allPatchPolyStrings) {
      //     auto id = p.first;
      //     fout << "// Patch id " << id << " (index " << patchIndex << "):\n";
      //     if (allPatchReplacementsPolyStrings[id].empty()) fout << "%";
      //     fout << "if (patchIndex < 0 || patchIndex == " << patchIndex << ") {\n";
      //     fout << "  if (before) { " << p.second.c_str() << " }\n";
      //     fout << "  else { " << allPatchReplacementsPolyStrings[id] << " }\n";
      //     fout << "}\n";
      //     patchIndex++;
      //   }
      // }

      if (!edits.isEmpty()) {
        edits.apply(tm);
      }

      auto facesAfter = tm.number_of_faces();
      if (verbose) {
        LOG(message_group::None, Location::NONE, "",
            "[fast-csg-remesh] Remeshed from %1$lu to %2$lu faces (%3$lu % improvement)", facesBefore, facesAfter,
            (facesBefore - facesAfter) * 100 / facesBefore);
      }
    } catch (const CGAL::Assertion_exception& e) {
      LOG(message_group::Error, Location::NONE, "", "CGAL error in remeshSplitFaces: %1$s", e.what());
    }
  }

private:

  bool isEdgeBetweenCoplanarFaces(const halfedge_descriptor& h) {
    auto& p = tm.point(tm.source(h));
    auto& q = tm.point(tm.target(h));
    auto& r = tm.point(tm.target(tm.next(h)));
    auto& s = tm.point(tm.target(tm.next(tm.opposite(h))));

    return CGAL::coplanar(p, q, r, s);
  }

  /*! Expand the patch of known faces to surrounding neighbours that pass the (fast) predicate. */
  bool floodFillPatch(
    std::set<face_descriptor>& facePatch,
    const std::function<bool(const halfedge_descriptor&)>& isHalfedgeOnBorder)
  {
    // Avoid recursion as its depth would be model-dependent.
    std::vector<face_descriptor> unprocessedFaces(facePatch.begin(), facePatch.end());


    while (!unprocessedFaces.empty()) {
      auto face = unprocessedFaces.back();
      unprocessedFaces.pop_back();

      CGAL::Halfedge_around_face_iterator<TriangleMesh> heIt, heEnd;
      for (boost::tie(heIt, heEnd) = halfedges_around_face(tm.halfedge(face), tm); heIt != heEnd; ++heIt) {
        auto he = *heIt;
        if (isHalfedgeOnBorder(he)) {
          continue;
        }

        auto neighbourFace = tm.face(tm.opposite(he));
        if (!facePatch.insert(neighbourFace).second) {
          continue;
        }

        unprocessedFaces.push_back(neighbourFace);
      }
    }

    return true;
  }

  /*! Returns true if the border is closed. */
  bool walkAroundPatch(halfedge_descriptor startingEdge,
                       const face_predicate& isFaceOnPatch,
                       std::vector<halfedge_descriptor>& borderOut)
  {
    auto currentEdge = startingEdge;
    borderOut.push_back(startingEdge);
    // std::set<vertex_descriptor> visitedVertices;

    std::unordered_set<vertex_descriptor> visitedVertices;

    while (tm.target(currentEdge) != tm.source(startingEdge)) {
      // visitedVertices.insert(tm.target(currentEdge));

      auto foundNext = false;
      CGAL::Halfedge_around_source_iterator<TriangleMesh> heIt, heEnd;
      for (boost::tie(heIt, heEnd) = halfedges_around_source(tm.target(currentEdge), tm); heIt != heEnd; ++heIt) {
        auto he = *heIt;
        if (he == tm.opposite(currentEdge)) {
          // Don't go back to where we just came from so quickly!
          continue;
        }
        // if (visitedVertices.find(tm.target(he)) != visitedVertices.end()) {
        //   continue;
        // }

        if (!isFaceOnPatch(tm.face(he))) {
          // This halfedge's face isn't on the patch, ignore it.
          continue;
        }

        if (isFaceOnPatch(tm.face(tm.opposite(he)))) {
          // Not a border edge as both faces are on the patch
          continue;
        }

        auto v = tm.target(he);
        if (!visitedVertices.insert(v).second) {
          continue;
        }


        // Edge is on the border of the patch.
        currentEdge = he;
        borderOut.push_back(currentEdge);
        foundNext = true;
        break;
      }
      if (!foundNext) {
        std::cerr << "Error: Failed to find the next edge to walk to along the patch border!\n";
        return false;
      }
    }

    return true;
  }
};

} // namespace CGALUtils

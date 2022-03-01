// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

// #define VERBOSE_REMESHING 1

#ifdef VERBOSE_REMESHING
  #include <boost/format.hpp>
  #include <random>
#endif
#include <iterator>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Kernel/global_functions.h>
#include <CGAL/Kernel/function_objects.h>

#include "cgalutils-mesh-edits.h"

namespace CGALUtils {

template <class Container, class Item>
bool contains(const Container& container, const Item& item) {
  return container.find(item) != container.end();
}

/*! Class that remeshes coplanar faces of a surface mesh.
 *
 * Please read the concepts about halfedge here:
 * https://doc.cgal.org/latest/Surface_mesh/index.html#title2
 *
 * This is useful for the results of corefinement operations, which result in
 * suboptimal meshes which growth compounds over several operations and makes
 * some models exponentially slow.
 *
 * Two faces with the same patch id *MUST* be coplanar, but need not be continuous
 * (technically the same patch id could result in different patches, e.g. if
 * during corefinement the same original face got split in disjoint sets of
 * descendants).
 *
 * Maximal patches of neighbouring coplanar faces are found with a flood-fill
 * algorithm starting from a given set of patch ids and using coplanarity tests
 * to merge patches with their neighbours.
 *
 * Patches that contain holes are skipped for now.
 *
 * Patch borders are found and we mark their collapsible vertices (those between
 * collinear edges). If exactly two neighbour patches agree, those vertices will
 * be dropped.
 *
 * Each patch is replaced with a polygon that espouses its borders, and we let
 * CGAL triangulate it.
 *
 * A new mesh is created with the edits, as in-place edits seem tricky.
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

#ifdef VERBOSE_REMESHING
    auto patchToPolyhedronStr = [&](auto& patchFaces, bool translucent) {
        std::vector<vertex_descriptor> vertices;
        std::map<vertex_descriptor, size_t> vertexIndex;

        std::ostringstream verticesOut;
        std::ostringstream facesOut;

        facesOut << "[\n";
        for (auto& face : patchFaces) {
          CGAL::Halfedge_around_face_iterator<TriangleMesh> heIt, heEnd;
          facesOut << "  [";
          auto first = true;
          for (boost::tie(heIt, heEnd) = halfedges_around_face(tm.halfedge(face), tm); heIt != heEnd; ++heIt) {
            auto he = *heIt;
            auto v = tm.source(he);
            auto it = vertexIndex.find(v);
            size_t idx;
            if (it == vertexIndex.end()) {
              idx = vertices.size();
              vertices.push_back(v);
              vertexIndex[v] = idx;
            } else {
              idx = it->second;
            }
            if (first) first = false;
            else facesOut << ", ";
            facesOut << idx;
          }
          facesOut << "],\n";
        }
        facesOut << "]";

        verticesOut << "[\n";
        for (auto v : vertices) {
          auto p = tm.point(v);
          verticesOut << "[" << CGAL::to_double(p.x()) << ", " << CGAL::to_double(p.y()) << ", " << CGAL::to_double(p.z()) << "],\n";
        }
        verticesOut << "]";

        std::ostringstream out;
        out << (translucent ? "%" : "") << "polyhedron(" << verticesOut.str().c_str() << ", " << facesOut.str().c_str() << ");";
        return out.str();
      };

    auto patchBorderToPolyhedronStr = [&](auto& borderPathVertices) {
        std::vector<vertex_descriptor> vertices;
        std::map<vertex_descriptor, size_t> vertexIndex;

        std::ostringstream verticesOut;
        std::ostringstream facesOut;

        facesOut << "[[";
        auto first = true;
        for (auto& v : borderPathVertices) {
          auto it = vertexIndex.find(v);
          size_t idx;
          if (it == vertexIndex.end()) {
            idx = vertices.size();
            vertices.push_back(v);
            vertexIndex[v] = idx;
          } else {
            idx = it->second;
          }
          if (first) first = false;
          else facesOut << ", ";
          facesOut << idx;
        }
        facesOut << "]]\n";

        verticesOut << "[\n";
        for (auto v : vertices) {
          auto p = tm.point(v);
          verticesOut << "[" << CGAL::to_double(p.x()) << ", " << CGAL::to_double(p.y()) << ", " << CGAL::to_double(p.z()) << "],\n";
        }
        verticesOut << "]";

        std::ostringstream out;
        out << "polyhedron(" << verticesOut.str().c_str() << ", " << facesOut.str().c_str() << ");";
        return out.str();
      };
#endif // VERBOSE_REMESHING

    class PatchData
    {
public:
      std::unordered_set<face_descriptor> patchFaces;
      std::unordered_map<PatchId, bool> isPatchIdMap;
      std::vector<halfedge_descriptor> borderEdges;
      std::vector<halfedge_descriptor> borderPath;
      std::vector<vertex_descriptor> borderPathVertices;
      std::unordered_set<halfedge_descriptor> borderPathEdges;

      void clear() {
        patchFaces.clear();
        isPatchIdMap.clear();
        borderEdges.clear();
        borderPath.clear();
        borderPathVertices.clear();
        borderPathEdges.clear();
      }
    };

    try {
      TriangleMeshEdits<TriangleMesh> edits;

      std::unordered_set<face_descriptor> facesProcessed;
      // Each patch border vertex to collapse should receive two marks to be collapsible.
      std::unordered_map<vertex_descriptor, size_t> collapsibleBorderVerticesMarks;

      auto remeshPatch = [&](PatchData& patch, PatchId id) {
        // This predicate assumes the halfedge's incident face is in the patch,
        // and tells whether the opposite face isn't.
        // It tries to keep expensive coplanarity tests to a minimum by
        // remembering what patch ids were already encountered.
        auto isHalfedgeOnBorder = [&](auto& he) {
            auto neighbourFace = tm.face(tm.opposite(he));
            if (!neighbourFace.is_valid()) {
              return true;
            }

            if (contains(facesProcessed, neighbourFace)) {
              // If we've already processed that face, it cannot be coplanar as
              // otherwise it would have engulfed the current face in its own
              // patch, so we're at a patch border right now.
              return true;
            }

            auto neighbourIdIt = faceToCoplanarPatchId.find(neighbourFace);
            if (neighbourIdIt == faceToCoplanarPatchId.end()) {
              std::cerr << "Unknown face, weird!";
              return true;
            }
            auto neighbourId = neighbourIdIt->second;

            auto isPatchIdIt = patch.isPatchIdMap.find(neighbourId);
            if (isPatchIdIt != patch.isPatchIdMap.end()) {
              auto isPatchId = isPatchIdIt->second;
              return !isPatchId;
            }

            auto isCoplanar = isEdgeBetweenCoplanarFaces(he);
            patch.isPatchIdMap[neighbourId] = isCoplanar;
            return !isCoplanar;
          };

        floodFillPatch(patch.patchFaces, isHalfedgeOnBorder);

        if (patch.patchFaces.size() < 2) {
          return false;
        }

        auto isFaceOnPatch = [&](auto& f) {
            return contains(patch.patchFaces, f);
          };

        for (auto& face : patch.patchFaces) {
          CGAL::Halfedge_around_face_iterator<TriangleMesh> heIt, heEnd;
          for (boost::tie(heIt, heEnd) = halfedges_around_face(tm.halfedge(face), tm); heIt != heEnd; ++heIt) {
            auto he = *heIt;
            if (isHalfedgeOnBorder(he)) {
              patch.borderEdges.push_back(he);
            }
          }
        }

        if (patch.borderEdges.empty()) {
          std::cerr << "Failed to find a border edge for patch " << id << "!\n";
          return false;
        }

        // Corefinement gives differently ordered meshes in release vs. debug builds.
        // (see https://github.com/CGAL/cgal/issues/6363).
        // To avoid remeshing them differently in these builds (and have stable
        // test expectations) we pick the "smallest" point to start the
        // border from. This comes at a a cost, but oh well!
        halfedge_descriptor borderEdge =
          *std::min_element(patch.borderEdges.begin(), patch.borderEdges.end(),
                            [&](auto& he1, auto& he2) {
          auto& v1 = tm.point(tm.source(he1));
          auto& v2 = tm.point(tm.source(he2));

          if (v1.x() < v2.x()) true;
          if (v1.x() != v2.x()) false;

          if (v1.y() < v2.y()) true;
          if (v1.y() != v2.y()) false;

          return v1.z() < v2.z();
        });

        if (!walkAroundPatch(borderEdge, isFaceOnPatch, patch.borderPath)) {
          LOG(message_group::Error, Location::NONE, "",
              "[fast-csg-remesh] Failed to collect path around patch faces, invalid mesh!");
          return false;
        }

        // TODO(ochafik): Find out when it's pointless to remesh (e.g. when no vertex can be collapsed or dropped because it's inside the patch).
        if (patch.borderPath.size() <= 3) {
          return false;
        }

        for (auto he : patch.borderPath) {
          patch.borderPathEdges.insert(he);
          patch.borderPathVertices.push_back(tm.target(he));
        }

        auto hasHoles = false;
        for (auto he : patch.borderEdges) {
          if (!contains(patch.borderPathEdges, he)) {
            // Found a border halfedge that's not in the border path we've walked around the patch.
            // This means the patch is holed / has more than one border: abort!!
            hasHoles = true;
            break;
          }
        }
        if (hasHoles) {
          if (verbose) {
            LOG(message_group::None, Location::NONE, "",
                "[fast-csg-remesh] Skipping remeshing of patch with %1$lu faces as it seems to have holes.", patch.borderPathEdges.size());
          }
          return false;
        }

        // TODO(ochafik): Ensure collapse happens on both sides of the border! (e.g. count of patches around vertex == 2)
        // auto collapsed = TriangleMeshEdits<TriangleMesh>::collapsePathWithConsecutiveCollinearEdges(borderPathVertices, tm);
        TriangleMeshEdits<TriangleMesh>::findCollapsibleVertices(patch.borderPathVertices, tm, [&](size_t index, vertex_descriptor v) {
          collapsibleBorderVerticesMarks[v]++;
        });

        // Cover the patch with a polygon. It will be triangulated when the
        // edits are applied.
        for (auto& face : patch.patchFaces) edits.removeFace(face);
        edits.addFace(patch.borderPathVertices);

        return true;
      };

#ifdef VERBOSE_REMESHING
      std::unordered_map<PatchId, std::string> allPatchPolyStrings;
      std::unordered_map<PatchId, std::string> allPatchReplacementsPolyStrings;
#endif // VERBOSE_REMESHING

      PatchData loopLocalPatchData;

      for (auto face : tm.faces()) {
        if (tm.is_removed(face)) {
          continue;
        }
        if (contains(facesProcessed, face)) {
          continue;
        }

        auto idIt = faceToCoplanarPatchId.find(face);
        if (idIt == faceToCoplanarPatchId.end()) {
          continue;
        }
        const auto id = idIt->second;
        if (!contains(patchesToRemesh, id)) {
          continue;
        }

        loopLocalPatchData.clear();
        auto& patch = loopLocalPatchData;
        patch.patchFaces.insert(face);
        patch.isPatchIdMap[id] = true;

        auto remeshed = remeshPatch(patch, id);

#ifdef VERBOSE_REMESHING
        allPatchPolyStrings[id] = patchToPolyhedronStr(patch.patchFaces, /* translucent= */ !remeshed);

        if (remeshed) {
          allPatchReplacementsPolyStrings[id] = patchBorderToPolyhedronStr(patch.borderPathVertices);
        }
#endif // VERBOSE_REMESHING

        for (auto& face : patch.patchFaces) {
          facesProcessed.insert(face);
        }
      }

      size_t collapsedVertexCount = 0;
      for (auto& p : collapsibleBorderVerticesMarks) {
        if (p.second != 2) {
          // If a vertex doesn't have 2 marks, then either the mesh has a
          // physical hole here (unsupported), or the neighbouring patch isn't
          // being remeshed (too bad, we don't want to spend too much time
          // remeshing all the patches), or the geometry isn't manifold (more
          // than two patches incident to that vertex with two collinear edges
          // around it)
          continue;
        }
        edits.removeVertex(p.first);
        collapsedVertexCount++;
      }

#if VERBOSE_REMESHING
      if (verbose) {
        LOG(message_group::None, Location::NONE, "",
            "[fast-csg-remesh] Collapsing %1$lu double-marked vertices", collapsedVertexCount);

        static size_t i = 0;
        std::ostringstream outName;
        outName << "patches " << i++ << ".scad";
        std::cout << "Writing " << outName.str().c_str() << "\n";

        std::ofstream fout(outName.str().c_str());
        fout << "before=true;\npatchIndex=-1;\n";
        size_t patchIndex = 0;

        std::mt19937 gen(123456789);
        std::uniform_int_distribution<> distrib(0, 255);

        for (auto& p : allPatchPolyStrings) {
          auto id = p.first;
          fout << "// Patch id " << id << " (index " << patchIndex << "):\n";
          fout << "color (\"#" << boost::format("%02x%02x%02x") % distrib(gen) % distrib(gen) % distrib(gen) << "\") {\n";
          if (allPatchReplacementsPolyStrings[id].empty()) fout << "%";
          fout << "  if (patchIndex < 0 || patchIndex == " << patchIndex << ") {\n";
          fout << "    if (before) { " << p.second.c_str() << " }\n";
          fout << "    else { " << allPatchReplacementsPolyStrings[id] << " }\n";
          fout << "  }\n}\n";
          patchIndex++;
        }
      }
#endif // VERBOSE_REMESHING

      if (!edits.isEmpty()) {
        edits.apply(tm);
      }

      auto facesAfter = tm.number_of_faces();
      if (verbose && facesBefore != facesAfter) {
        LOG(message_group::None, Location::NONE, "",
            "[fast-csg-remesh] Remeshed from %1$lu to %2$lu faces (%3$lu % improvement)", facesBefore, facesAfter,
            (facesBefore - facesAfter) * 100 / facesBefore);
      }
    } catch (const CGAL::Assertion_exception& e) {
      LOG(message_group::Error, Location::NONE, "", "CGAL error in remeshSplitFaces: %1$s", e.what());
    }
  }

  bool isEdgeBetweenCoplanarFaces(const halfedge_descriptor& h) const {
    auto& p = tm.point(tm.source(h));
    auto& q = tm.point(tm.target(h));
    auto& r = tm.point(tm.target(tm.next(h)));
    auto& s = tm.point(tm.target(tm.next(tm.opposite(h))));

    return CGAL::coplanar(p, q, r, s);
  }

  /*! Expand the patch of known faces to surrounding neighbours that pass the (fast) predicate. */
  bool floodFillPatch(
    std::unordered_set<face_descriptor>& facePatch,
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
                       std::vector<halfedge_descriptor>& borderOut) const
  {
    auto currentEdge = startingEdge;
    borderOut.push_back(startingEdge);

    std::unordered_set<vertex_descriptor> visitedVertices;

    while (tm.target(currentEdge) != tm.source(startingEdge)) {
      auto foundNext = false;
      CGAL::Halfedge_around_source_iterator<TriangleMesh> heIt, heEnd;
      for (boost::tie(heIt, heEnd) = halfedges_around_source(tm.target(currentEdge), tm); heIt != heEnd; ++heIt) {
        auto he = *heIt;
        if (he == tm.opposite(currentEdge)) {
          // Don't go back to where we just came from so quickly!
          continue;
        }

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

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

    try {
      TriangleMeshEdits<TriangleMesh> edits;
      std::unordered_set<PatchId> patchesProcessed;
      patchesProcessed.reserve(patchesToRemesh.size());

      std::set<face_descriptor> loopLocalPatchFaces;
      std::map<PatchId, bool> loopLocalIsPatchIdMap;
      std::vector<vertex_descriptor> loopLocalBorderVertices;

      for (auto face : tm.faces()) {
        auto idIt = faceToCoplanarPatchId.find(face);
        if (idIt == faceToCoplanarPatchId.end()) {
          continue;
        }
        const auto id = idIt->second;

        if (patchesToRemesh.find(id) == patchesToRemesh.end()) {
          continue;
        }
        if (patchesProcessed.find(id) != patchesProcessed.end()) {
          continue;
        }
        patchesProcessed.insert(id);

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

            // We're on the border
            return true;
          };

        floodFillPatch(patchFaces, isHalfedgeOnBorder);

        if (patchFaces.size() < 2) {
          continue;
        }

        auto isFaceOnPatch = [&](auto& f) {
            return patchFaces.find(f) != patchFaces.end();
          };

        halfedge_descriptor borderEdge;
        for (auto& face : patchFaces) {
          CGAL::Halfedge_around_face_iterator<TriangleMesh> heIt, heEnd;
          for (boost::tie(heIt, heEnd) = halfedges_around_face(tm.halfedge(face), tm); heIt != heEnd; ++heIt) {
            auto he = *heIt;
            if (isHalfedgeOnBorder(he)) {
              borderEdge = he;
              break;
            }
          }
        }

        if (!borderEdge.is_valid()) {
          std::cerr << "Failed to find a border edge for patch " << id << "!\n";
          continue;
        }

        std::vector<halfedge_descriptor> borderPath;
        if (!walkAroundPatch(borderEdge, isFaceOnPatch, borderPath)) {
          LOG(message_group::Error, Location::NONE, "",
              "[fast-csg-remesh] Failed to collect path around patch faces, invalid mesh!");
          return;
        }

        if (borderPath.size() <= 3) {
          continue;
        }

        loopLocalBorderVertices.clear();
        auto& borderVertices = loopLocalBorderVertices;
        for (auto he : borderPath) {
          borderVertices.push_back(tm.target(he));
        }

        // Only remesh patches that have consecutive collinear edges.
        auto lengthBefore = borderVertices.size();
        if (TriangleMeshEdits<TriangleMesh>::collapsePathWithConsecutiveCollinearEdges(borderVertices, tm)) {
          auto lengthAfter = borderVertices.size();
          std::cerr << "Collapsed path around patch " << id << " (" << patchFaces.size() << " faces) from " << lengthBefore << " to " << lengthAfter << " vertices\n";

          for (auto& face : patchFaces) {
            edits.removeFace(face);
          }
          // Cover the patch with a polygon. It will be triangulated when the
          // edits are applied.
          edits.addFace(borderVertices);
        }
      }

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
        if (facePatch.find(neighbourFace) != facePatch.end()) {
          continue;
        }

        facePatch.insert(neighbourFace);
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
        if (visitedVertices.find(v) != visitedVertices.end()) {
          continue;
        }
        visitedVertices.insert(v);

        std::cerr << "Edge " << he << " between " << tm.face(he) << " and " << tm.face(tm.opposite(he)) << " is on the border!\n";

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

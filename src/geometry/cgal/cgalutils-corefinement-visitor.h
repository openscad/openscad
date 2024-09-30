// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include "geometry/cgal/cgalutils-coplanar-faces-remesher.h"

#include <cstddef>
#include <vector>

namespace CGALUtils {

namespace internal {

template <typename Pt>
void forceExact(Pt& point)
{
  CGAL::exact(point.x());
  CGAL::exact(point.y());
  CGAL::exact(point.z());
}

/*!
 * Corefinement visitor that serves two purposes:
 * - Track the ancestors of all resulting faces in the original meshes, and
 *   remesh those descendants with their neighbouring coplanar faces. All
 *   original faces have a patch id, which is inherited by copy or splitting.
 * - Make any new numbers exact
 */
template <typename TriangleMesh>
struct CorefinementVisitorDelegate_ {
private:
  using GT = boost::graph_traits<TriangleMesh>;
  using face_descriptor = typename GT::face_descriptor;
  using halfedge_descriptor = typename GT::halfedge_descriptor;
  using edge_descriptor = typename GT::edge_descriptor;
  using vertex_descriptor = typename GT::vertex_descriptor;

  using PatchId = size_t; // Starting from 1

  TriangleMesh *mesh1_, *mesh2_, *meshOut_;

  // The following fields are scoped by mesh ([mesh1_, mesh2_, meshOut_]).
  // If meshOut_ == mesh1_ (or mesh2_), the third element of each of these
  // fields will go unused, as all accesses are using the index given by indexOf

  std::unordered_map<face_descriptor, PatchId> faceToPatchId_[3];
  std::unordered_set<PatchId> splitPatchIds_;

  face_descriptor faceBeingSplit_[3];
  std::vector<face_descriptor> facesBeingCreated_[3];

  PatchId getPatchId(const face_descriptor& f, const TriangleMesh& tm) {
    auto id = faceToPatchId_[indexOf(tm)][f];
    CGAL_assertion(id);
    return id;
  }

  void setPatchId(const face_descriptor& f, const TriangleMesh& tm, PatchId id) {
    CGAL_assertion(id);
    faceToPatchId_[indexOf(tm)][f] = id;
  }

  size_t indexOf(const TriangleMesh& tm) {
    if (&tm == mesh1_) return 0;
    if (&tm == mesh2_) return 1;
    if (&tm == meshOut_) return 2;
    CGAL_assertion(false && "Unknown mesh");
    throw 0;
  }

public:
  CorefinementVisitorDelegate_(TriangleMesh & m1, TriangleMesh & m2, TriangleMesh & mout)
    : mesh1_(&m1), mesh2_(&m2), meshOut_(&mout)
  {
    TriangleMesh *meshes[3] = {&m1, &m2, &mout};

    // Give each existing face its own global patch id.
    PatchId nextPatchId = 1;
    for (std::size_t mi = 0; mi < 3; mi++) {
      auto& m = *meshes[mi];

      if (mi != indexOf(m)) {
        // &mout is often == &m1, so don't reassign its patch ids!
        continue;
      }

      for (auto f : m.faces()) setPatchId(f, m, nextPatchId++);
    }
  }

  void remeshSplitFaces(TriangleMesh& tm)
  {
    auto mi = indexOf(tm);
    CoplanarFacesRemesher<TriangleMesh> remesher(tm);
    remesher.remesh(faceToPatchId_[mi], splitPatchIds_);
  }

  void after_face_copy(face_descriptor f_old, const TriangleMesh& m_old, face_descriptor f_new,
                       TriangleMesh& m_new)
  {
    setPatchId(f_new, m_new, getPatchId(f_old, m_old));
  }

  void before_subface_creations(face_descriptor f_split, TriangleMesh& tm)
  {
    auto mi = indexOf(tm);
    faceBeingSplit_[mi] = f_split;
    facesBeingCreated_[mi].clear();
  }

  void after_subface_created(face_descriptor fi, TriangleMesh& tm)
  {
    auto mi = indexOf(tm);
    auto id = getPatchId(faceBeingSplit_[mi], tm);
    setPatchId(fi, tm, id);

#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(5, 4, 0)
    facesBeingCreated_[mi].push_back(fi);
#endif
  }

  void after_subface_creations(TriangleMesh& tm)
  {
    auto mi = indexOf(tm);
    auto id = getPatchId(faceBeingSplit_[mi], tm);
    splitPatchIds_.insert(id);

    // From CGAL 5.4 on we rely on new_vertex_added instead.
#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(5, 4, 0)
    for (auto& fi : facesBeingCreated_[mi]) {
      CGAL::Vertex_around_face_iterator<TriangleMesh> vbegin, vend;
      for (boost::tie(vbegin, vend) = vertices_around_face(tm.halfedge(fi), tm); vbegin != vend;
            ++vbegin) {
        forceExact(tm.point(*vbegin));
      }
    }
#endif // if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(5, 4, 0)
  }

  // Note: only called in CGAL 5.4+
  void new_vertex_added(std::size_t /*node_id*/, vertex_descriptor vh, const TriangleMesh& tm)
  {
    forceExact(tm.point(vh));
  }

};

} // namespace internal

/**
 * This corefinement visitor can be used for all of the (lhs, rhs, output) meshes
 * involved in corefinement. It will be copied but refers to a single delegate
 * that gets the callbacks from all of the copies.
 */
template <typename TriangleMesh>
struct CorefinementVisitor : public PMP::Corefinement::Default_visitor<TriangleMesh> {
  using GT = boost::graph_traits<TriangleMesh>;
  using face_descriptor = typename GT::face_descriptor;
  using vertex_descriptor = typename GT::vertex_descriptor;

  using Delegate = internal::CorefinementVisitorDelegate_<TriangleMesh>;

  std::shared_ptr<Delegate> delegate_;

  CorefinementVisitor(TriangleMesh & m1, TriangleMesh & m2, TriangleMesh & mout)
    : delegate_(std::make_shared<Delegate>(m1, m2, mout))
  {
  }

  void remeshSplitFaces(TriangleMesh& tm) {
    delegate_->remeshSplitFaces(tm);
  }

  void before_subface_creations(face_descriptor f_old, TriangleMesh& tm) {
    delegate_->before_subface_creations(f_old, tm);
  }
  void after_subface_creations(TriangleMesh& tm) {
    delegate_->after_subface_creations(tm);
  }
  void after_subface_created(face_descriptor f_new, TriangleMesh& tm) {
    delegate_->after_subface_created(f_new, tm);
  }
  void after_face_copy(face_descriptor f_old, const TriangleMesh& old_tm, face_descriptor f_new, TriangleMesh& new_tm) {
    delegate_->after_face_copy(f_old, old_tm, f_new, new_tm);
  }
  void new_vertex_added(std::size_t node_id, vertex_descriptor vh, const TriangleMesh& tm) {
    delegate_->new_vertex_added(node_id, vh, tm);
  }
};

} // namespace CGALUtils

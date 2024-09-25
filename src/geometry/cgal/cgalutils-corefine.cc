// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"

#include "geometry/cgal/cgalutils-corefinement-visitor.h"

namespace CGALUtils {

#define COREFINEMENT_FUNCTION(functionName, cgalFunctionName) \
        template <class TriangleMesh> \
        bool functionName(TriangleMesh &lhs, TriangleMesh &rhs, TriangleMesh &out) \
        { \
          CorefinementVisitor<TriangleMesh> visitor(lhs, rhs, out); \
          auto param = PMP::parameters::visitor(visitor); \
          auto result = cgalFunctionName(lhs, rhs, out, param, param, param); \
          visitor.remeshSplitFaces(out); \
          return result; \
        }

COREFINEMENT_FUNCTION(corefineAndComputeUnion, PMP::corefine_and_compute_union);
COREFINEMENT_FUNCTION(corefineAndComputeIntersection, PMP::corefine_and_compute_intersection);
COREFINEMENT_FUNCTION(corefineAndComputeDifference, PMP::corefine_and_compute_difference);

template bool corefineAndComputeUnion(CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out);
template bool corefineAndComputeIntersection(CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out);
template bool corefineAndComputeDifference(CGAL_HybridMesh& lhs, CGAL_HybridMesh& rhs, CGAL_HybridMesh& out);

} // namespace CGALUtils


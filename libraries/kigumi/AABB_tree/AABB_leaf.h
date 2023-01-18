#pragma once

#include <CGAL/Bbox_3.h>

namespace kigumi {

class AABB_leaf {
  using Bbox = CGAL::Bbox_3;

 public:
  explicit AABB_leaf(const Bbox& bbox) : bbox_(bbox) {}

  const Bbox& bbox() const { return bbox_; }

 private:
  const Bbox bbox_;
};

}  // namespace kigumi

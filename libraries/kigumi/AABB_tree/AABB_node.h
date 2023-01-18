#pragma once

#include <CGAL/Bbox_3.h>

namespace kigumi {

template <class Leaf>
class AABB_node {
  using Bbox = CGAL::Bbox_3;

 public:
  const Bbox& bbox() const { return bbox_; }

  const AABB_node* left_node() const { return static_cast<const AABB_node*>(left_); }

  const AABB_node* right_node() const { return static_cast<const AABB_node*>(right_); }

  const Leaf* left_leaf() const { return static_cast<const Leaf*>(left_); }

  const Leaf* right_leaf() const { return static_cast<const Leaf*>(right_); }

  void set_bbox(const Bbox& bbox) { bbox_ = bbox; }

  void set_left_node(const AABB_node* node) { left_ = node; }

  void set_right_node(const AABB_node* node) { right_ = node; }

  void set_left_leaf(const Leaf* leaf) { left_ = leaf; }

  void set_right_leaf(const Leaf* leaf) { right_ = leaf; }

 private:
  Bbox bbox_;
  const void* left_{nullptr};
  const void* right_{nullptr};
};

}  // namespace kigumi

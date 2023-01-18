#pragma once

#include <CGAL/Bbox_3.h>
#include <kigumi/AABB_tree/AABB_node.h>
#include <omp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>

namespace kigumi {

template <class Leaf>
class AABB_tree {
  using Bbox = CGAL::Bbox_3;
  using Node = AABB_node<Leaf>;
  using Node_iterator = typename std::vector<Node>::iterator;
  using Leaf_iterator = typename std::vector<Leaf>::const_iterator;
  using Leaf_ptr = const Leaf*;
  using Leaf_ptr_iterator = typename std::vector<Leaf_ptr>::iterator;

 public:
  explicit AABB_tree(std::vector<Leaf>&& leaves) : leaves_(std::move(leaves)) {
    auto num_leaves = leaves_.size();
    if (num_leaves == 0) {
      return;
    }

    if (num_leaves == 1) {
      root_ = &leaves_.at(0);
      return;
    }

    nodes_.resize(num_leaves - 1);
    std::vector<Leaf_ptr> leaf_ptrs;
    leaf_ptrs.reserve(num_leaves);
    for (auto& leaf : leaves_) {
      leaf_ptrs.push_back(&leaf);
    }

    auto root = &nodes_.at(0);
    root->set_bbox(bbox_from_leaves(leaves_.begin(), leaves_.end()));
    root_ = root;

    build(nodes_.begin(), leaf_ptrs.begin(), leaf_ptrs.end(), 0);
  }

  template <class DoIntersect, class OutputIterator, class Query>
  void get_intersecting_leaves(OutputIterator leaves, const Query& query) const {
    auto num_leaves = leaves_.size();

    switch (num_leaves) {
      case 0:
        return;

      case 1:
        if (DoIntersect::do_intersect(root_leaf()->bbox(), query)) {
          *leaves++ = root_leaf();
        }
        return;

      default:
        traverse<DoIntersect>(leaves, num_leaves, query, root_node());
        return;
    }
  }

 private:
  // NOLINTNEXTLINE(misc-no-recursion)
  void build(Node_iterator node_it, Leaf_ptr_iterator leaves_begin, Leaf_ptr_iterator leaves_end,
             int node_depth) {
    auto num_leaves = static_cast<std::size_t>(std::distance(leaves_begin, leaves_end));

    switch (num_leaves) {
      case 2:  // left: leaf, right: leaf
        node_it->set_left_leaf(*leaves_begin);
        node_it->set_right_leaf(*(leaves_begin + 1));

        node_it->set_bbox(node_it->left_leaf()->bbox() + node_it->right_leaf()->bbox());
        break;

      case 3:  // left: leaf, right: node
      {
        auto right_node_it = node_it + 1;
        auto split_axis = bbox_longest_axis(node_it->bbox());
        std::sort(leaves_begin, leaves_end, [split_axis, this](auto a, auto b) {
          return bbox_center(a->bbox()).at(split_axis) < bbox_center(b->bbox()).at(split_axis);
        });

        node_it->set_left_leaf(*leaves_begin);
        node_it->set_right_node(&*right_node_it);

        build(right_node_it, leaves_begin + 1, leaves_end, node_depth + 1);
        node_it->set_bbox(node_it->left_leaf()->bbox() + node_it->right_node()->bbox());
        break;
      }

      default:  // left: node, right: node
      {
        auto num_left_leaves = num_leaves / 2;
        auto left_node_it = node_it + 1;
        // The left tree requires (num_left_leaves - 1) nodes.
        auto right_node_it = node_it + num_left_leaves;
        auto split_axis = bbox_longest_axis(node_it->bbox());
        std::nth_element(leaves_begin, leaves_begin + num_left_leaves, leaves_end,
                         [split_axis, this](auto a, auto b) {
                           return bbox_center(a->bbox()).at(split_axis) <
                                  bbox_center(b->bbox()).at(split_axis);
                         });

        node_it->set_left_node(&*left_node_it);
        node_it->set_right_node(&*right_node_it);

#pragma omp parallel sections if (node_depth < concurrency_depth_limit_)
        {
#pragma omp section
          build(left_node_it, leaves_begin, leaves_begin + num_left_leaves, node_depth + 1);
#pragma omp section
          build(right_node_it, leaves_begin + num_left_leaves, leaves_end, node_depth + 1);
        }
        node_it->set_bbox(node_it->left_node()->bbox() + node_it->right_node()->bbox());
        break;
      }
    }
  }

  template <class DoIntersect, class OutputIterator, class Query>
  // NOLINTNEXTLINE(misc-no-recursion)
  void traverse(OutputIterator leaves, std::size_t num_leaves, const Query& query,
                const Node* node) const {
    switch (num_leaves) {
      case 2:  // left: leaf, right: leaf
        if (DoIntersect::do_intersect(node->left_leaf()->bbox(), query)) {
          *leaves++ = node->left_leaf();
        }
        if (DoIntersect::do_intersect(node->right_leaf()->bbox(), query)) {
          *leaves++ = node->right_leaf();
        }
        break;

      case 3:  // left: leaf, right: node
        if (DoIntersect::do_intersect(node->left_leaf()->bbox(), query)) {
          *leaves++ = node->left_leaf();
        }
        if (DoIntersect::do_intersect(node->right_node()->bbox(), query)) {
          traverse<DoIntersect>(leaves, 2, query, node->right_node());
        }
        break;

      default:  // left: node, right: node
      {
        auto num_left_leaves = num_leaves / 2;
        if (DoIntersect::do_intersect(node->left_node()->bbox(), query)) {
          traverse<DoIntersect>(leaves, num_left_leaves, query, node->left_node());
        }
        if (DoIntersect::do_intersect(node->right_node()->bbox(), query)) {
          traverse<DoIntersect>(leaves, num_leaves - num_left_leaves, query, node->right_node());
        }
        break;
      }
    }
  }

  const Node* root_node() const { return static_cast<const Node*>(root_); }

  const Leaf* root_leaf() const { return static_cast<const Leaf*>(root_); }

  std::array<double, 3> bbox_center(const Bbox& bbox) {
    return {(bbox.xmax() + bbox.xmin()) / 2.0, (bbox.ymax() + bbox.ymin()) / 2.0,
            (bbox.zmax() + bbox.zmin()) / 2.0};
  }

  Bbox bbox_from_leaves(Leaf_iterator leaves_begin, Leaf_iterator leaves_end) {
    auto bbox = leaves_begin->bbox();
    for (auto it = leaves_begin + 1; it != leaves_end; ++it) {
      bbox = bbox + it->bbox();
    }
    return bbox;
  }

  int bbox_longest_axis(const Bbox& bbox) {
    std::array<double, 3> lengths{bbox.xmax() - bbox.xmin(), bbox.ymax() - bbox.ymin(),
                                  bbox.zmax() - bbox.zmin()};
    return static_cast<int>(
        std::distance(lengths.begin(), std::max_element(lengths.begin(), lengths.end())));
  }

  const int concurrency_depth_limit_{static_cast<int>(std::log2(omp_get_max_threads()))};
  const std::vector<Leaf> leaves_;
  std::vector<Node> nodes_;
  const void* root_{nullptr};
};

}  // namespace kigumi

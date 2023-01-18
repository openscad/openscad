#pragma once

#include <kigumi/Mesh_handles.h>

#include <boost/iterator/iterator_facade.hpp>
#include <vector>

namespace kigumi {

class Face_around_edge_iterator
    : public boost::iterator_facade<Face_around_edge_iterator, Face_handle,
                                    boost::forward_traversal_tag, Face_handle> {
  using Index_iterator = std::vector<Face_handle>::const_iterator;

 public:
  Face_around_edge_iterator() = default;

  Face_around_edge_iterator(Index_iterator i_it, Index_iterator i_end, Index_iterator j_it,
                            Index_iterator j_end)
      : i_it_(i_it), i_end_(i_end), j_it_(j_it), j_end_(j_end) {
    while (i_it_ != i_end_ && j_it_ != j_end_ && *i_it_ != *j_it_) {
      if (*i_it_ < *j_it_) {
        ++i_it_;
      } else {
        ++j_it_;
      }
    }

    if (i_it_ == i_end_ || j_it_ == j_end_) {
      i_it_ = i_end_;
      j_it_ = j_end_;
    }
  }

 private:
  friend class boost::iterator_core_access;

  void increment() {
    if (i_it_ == i_end_) {
      return;
    }

    ++i_it_;
    ++j_it_;

    while (i_it_ != i_end_ && j_it_ != j_end_ && *i_it_ != *j_it_) {
      if (*i_it_ < *j_it_) {
        ++i_it_;
      } else {
        ++j_it_;
      }
    }

    if (i_it_ == i_end_ || j_it_ == j_end_) {
      i_it_ = i_end_;
      j_it_ = j_end_;
    }
  }

  bool equal(const Face_around_edge_iterator& other) const { return i_it_ == other.i_it_; }

  Face_handle dereference() const { return *i_it_; }

  Index_iterator i_it_;
  Index_iterator i_end_;
  Index_iterator j_it_;
  Index_iterator j_end_;
};

class Face_iterator : public boost::iterator_facade<Face_iterator, Face_handle,
                                                    boost::forward_traversal_tag, Face_handle> {
 public:
  Face_iterator() = default;

  explicit Face_iterator(Face_handle fh) : fh_(fh) {}

 private:
  friend class boost::iterator_core_access;

  void increment() { ++fh_.i; }

  bool equal(const Face_iterator& other) const { return fh_ == other.fh_; }

  Face_handle dereference() const { return fh_; }

  Face_handle fh_{0};
};

class Face_around_face_iterator
    : public boost::iterator_facade<Face_around_face_iterator, Face_handle,
                                    boost::forward_traversal_tag, Face_handle> {
 public:
  Face_around_face_iterator() = default;

  Face_around_face_iterator(Face_handle fh, Face_around_edge_iterator it1,
                            Face_around_edge_iterator end1, Face_around_edge_iterator it2,
                            Face_around_edge_iterator end2, Face_around_edge_iterator it3,
                            Face_around_edge_iterator end3)
      : fh_(fh), it1_(it1), end1_(end1), it2_(it2), end2_(end2), it3_(it3), end3_(end3) {}

 private:
  friend class boost::iterator_core_access;

  void increment() {
    while (true) {
      if (it1_ != end1_) {
        ++it1_;
      } else if (it2_ != end2_) {
        ++it2_;
      } else if (it3_ != end3_) {
        ++it3_;
      } else {
        break;
      }

      if (dereference() != fh_) {
        break;
      }
    }
  }

  bool equal(const Face_around_face_iterator& other) const {
    return it1_ == other.it1_ && it2_ == other.it2_ && it3_ == other.it3_;
  }

  Face_handle dereference() const {
    if (it1_ != end1_) {
      return *it1_;
    }
    if (it2_ != end2_) {
      return *it2_;
    }
    if (it3_ != end3_) {
      return *it3_;
    }
    return Face_handle{};
  }

  Face_handle fh_;
  Face_around_edge_iterator it1_;
  Face_around_edge_iterator end1_;
  Face_around_edge_iterator it2_;
  Face_around_edge_iterator end2_;
  Face_around_edge_iterator it3_;
  Face_around_edge_iterator end3_;
};

}  // namespace kigumi

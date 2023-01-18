#pragma once

#include <kigumi/Mixed_mesh.h>

#include <queue>
#include <stdexcept>

namespace kigumi {

template <class K>
class Face_tag_propagator {
 public:
  explicit Face_tag_propagator(Mixed_mesh<K>& m, const std::unordered_set<Edge>& border)
      : m_(m), border_(border) {
    for (auto fh : m_.faces()) {
      auto tag = m_.data(fh).tag;
      if (tag == Face_tag::Intersection || tag == Face_tag::Union) {
        queue_.push(fh);
      }
    }

    propagate();
  }

  Face_tag_propagator(Mixed_mesh<K>& m, const std::unordered_set<Edge>& border, Face_handle seed)
      : m_(m), border_(border) {
    auto tag = m_.data(seed).tag;
    if (tag == Face_tag::Intersection || tag == Face_tag::Union) {
      queue_.push(seed);
    } else {
      throw std::runtime_error("seed face is not tagged as intersection or union");
    }

    propagate();
  }

 private:
  void propagate() {
    while (!queue_.empty()) {
      auto fh = queue_.front();
      queue_.pop();

      auto tag = m_.data(fh).tag;
      for (auto fh2 : m_.faces_around_face(fh, border_)) {
        if (m_.data(fh2).tag == Face_tag::Unknown) {
          m_.data(fh2).tag = tag;
          queue_.push(fh2);
        }
      }
    }
  }

  Mixed_mesh<K>& m_;
  const std::unordered_set<Edge>& border_;
  std::queue<Face_handle> queue_;
};

}  // namespace kigumi

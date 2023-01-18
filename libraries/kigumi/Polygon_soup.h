#pragma once

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/IO/polygon_soup_io.h>
#include <kigumi/AABB_tree/AABB_leaf.h>
#include <kigumi/AABB_tree/AABB_tree.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace kigumi {

template <class K>
class Polygon_soup {
  using Face = std::array<std::size_t, 3>;
  using Point = typename K::Point_3;
  using Triangle = typename K::Triangle_3;

 public:
  class Leaf : public AABB_leaf {
   public:
    Leaf(const Triangle& tri, std::size_t face_index)
        : AABB_leaf(tri.bbox()), face_index_(face_index) {}

    std::size_t face_index() const { return face_index_; }

   private:
    std::size_t face_index_;
  };

  Polygon_soup() = default;

  ~Polygon_soup() = default;

  Polygon_soup(const Polygon_soup& other) : points_(other.points_), faces_(other.faces_) {}

  Polygon_soup(Polygon_soup&& other) noexcept
      : points_(std::move(other.points_)),
        faces_(std::move(other.faces_)),
        aabb_tree_(std::move(other.aabb_tree_)) {}

  Polygon_soup& operator=(const Polygon_soup& other) {
    if (this != &other) {
      points_ = other.points_;
      faces_ = other.faces_;
      aabb_tree_.reset();
    }
    return *this;
  }

  Polygon_soup& operator=(Polygon_soup&& other) noexcept {
    points_ = std::move(other.points_);
    faces_ = std::move(other.faces_);
    aabb_tree_ = std::move(other.aabb_tree_);
    return *this;
  }

  explicit Polygon_soup(const std::string& filename) {
    // CGAL::IO::read_OBJ does not support std::vector<std::array<...>>.
    std::vector<std::vector<std::size_t>> faces;
    CGAL::IO::read_polygon_soup(filename, points_, faces);

    faces_.reserve(faces.size());
    for (const auto& face : faces) {
      if (face.size() != 3) {
        throw std::runtime_error("not a triangle mesh");
      }
      faces_.push_back({face[0], face[1], face[2]});
    }
  }

  Polygon_soup(std::vector<Point>&& points, std::vector<Face>&& faces)
      : points_(std::move(points)), faces_(std::move(faces)) {}

  Polygon_soup(std::vector<Point>& points, std::vector<Face>& faces)
      : points_(points), faces_(faces) {}

  void save(const std::string& filename) {
    using Epick = CGAL::Exact_predicates_inexact_constructions_kernel;

    std::vector<Epick::Point_3> points;
    points.reserve(points_.size());
    for (const auto& p : points_) {
      points.emplace_back(CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z()));
    }

    CGAL::IO::write_polygon_soup(filename, points, faces_);
  }

  std::size_t num_faces() const { return faces_.size(); }

  const std::vector<Point>& points() const { return points_; }

  const std::vector<Face>& faces() const { return faces_; }

  Triangle triangle(std::size_t i) const {
    const auto& face = faces_.at(i);
    const auto& p = points_.at(face[0]);
    const auto& q = points_.at(face[1]);
    const auto& r = points_.at(face[2]);
    return {p, q, r};
  }

  void invert() {
    for (auto& face : faces_) {
      std::swap(face[1], face[2]);
    }
  }

  const AABB_tree<Leaf>& aabb_tree() const {
    std::lock_guard<std::mutex> lk(aabb_tree_mutex_);

    if (!aabb_tree_) {
      std::vector<Leaf> leaves;
      for (std::size_t i = 0; i < faces_.size(); ++i) {
        leaves.emplace_back(triangle(i), i);
      }
      aabb_tree_ = std::make_unique<AABB_tree<Leaf>>(std::move(leaves));
    }

    return *aabb_tree_;
  }

 private:
  std::vector<Point> points_;
  std::vector<Face> faces_;
  mutable std::unique_ptr<AABB_tree<Leaf>> aabb_tree_;
  mutable std::mutex aabb_tree_mutex_;
};

}  // namespace kigumi

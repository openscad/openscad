#pragma once

#include <CGAL/intersection_3.h>
#include <kigumi/Face_pair_finder.h>
#include <kigumi/Polygon_soup.h>
#include <kigumi/Triangulator.h>

#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kigumi {

template <class K>
class Corefine {
  using Point = typename K::Point_3;
  using Segment = typename K::Segment_3;
  using Triangle = typename K::Triangle_3;
  using Intersection =
      decltype(CGAL::intersection(std::declval<Triangle>(), std::declval<Triangle>()));

 public:
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  Corefine(const Polygon_soup<K>& left, const Polygon_soup<K>& right) : left_(left), right_(right) {
    Face_pair_finder finder(left_, right_);
    auto pairs = finder.find_face_pairs();

    std::vector<Intersection_info> infos;

#pragma omp parallel
    {
      std::vector<Intersection_info> local_infos;

#pragma omp for schedule(guided)
      for (std::size_t i = 0; i < pairs.size(); ++i) {
        auto [left_face, right_face] = pairs.at(i);
        auto left_tri = left_.triangle(left_face);
        auto right_tri = right_.triangle(right_face);
        if( !left_tri.is_degenerate() && !right_tri.is_degenerate() ) {
          auto result = CGAL::intersection(left_tri, right_tri);
          if (result) {
            local_infos.emplace_back(left_face, right_face, std::move(result));
          }
        }
      }

#pragma omp critical
      infos.insert(infos.end(), std::make_move_iterator(local_infos.begin()),
                   std::make_move_iterator(local_infos.end()));
    }

    for (const auto& info : infos) {
      if (!left_triangulators_.contains(info.left_face)) {
        auto tri = left_.triangle(info.left_face);
        if( tri.is_degenerate() ) {
          continue;
        }
        left_triangulators_.emplace(info.left_face, Triangulator<K>(tri));
      }
      if (!right_triangulators_.contains(info.right_face)) {
        auto tri = right_.triangle(info.right_face);
        if( tri.is_degenerate() ) {
          continue;
        }
        right_triangulators_.emplace(info.right_face, Triangulator<K>(tri));
      }
    }

    std::sort(infos.begin(), infos.end(),
              [](const auto& a, const auto& b) { return a.left_face < b.left_face; });

    std::vector<std::size_t> left_face_starts;
    for (std::size_t i = 0; i < infos.size(); ++i) {
      if (i == 0 || infos.at(i).left_face != infos.at(i - 1).left_face) {
        left_face_starts.push_back(i);
      }
    }
    left_face_starts.push_back(infos.size());

#pragma omp parallel for schedule(guided)
    for (std::size_t i = 0; i < left_face_starts.size() - 1; ++i) {
      auto left_face = infos.at(left_face_starts.at(i)).left_face;
      auto tri = left_.triangle(left_face);
      if( tri.is_degenerate() ) {
        continue;
      }
      auto& triangulator = left_triangulators_.at(left_face);

      for (auto j = left_face_starts.at(i); j < left_face_starts.at(i + 1); ++j) {
        try {
          insert_intersection(triangulator, infos.at(j).intersection);
        } catch (const typename Triangulator<K>::Intersection_of_constraints_exception&) {
        }
      }
    }

    std::sort(infos.begin(), infos.end(),
              [](const auto& a, const auto& b) { return a.right_face < b.right_face; });

    std::vector<std::size_t> right_face_starts;
    for (std::size_t i = 0; i < infos.size(); ++i) {
      if (i == 0 || infos.at(i).right_face != infos.at(i - 1).right_face) {
        right_face_starts.push_back(i);
      }
    }
    right_face_starts.push_back(infos.size());

#pragma omp parallel for schedule(guided)
    for (std::size_t i = 0; i < right_face_starts.size() - 1; ++i) {
      auto right_face = infos.at(right_face_starts.at(i)).right_face;
      auto tri = right_.triangle(right_face);
      if( tri.is_degenerate() ) {
        continue;
      }
      auto& triangulator = right_triangulators_.at(right_face);

      for (auto j = right_face_starts.at(i); j < right_face_starts.at(i + 1); ++j) {
        try {
          insert_intersection(triangulator, infos.at(j).intersection);
        } catch (const typename Triangulator<K>::Intersection_of_constraints_exception&) {
        }
      }
    }
  }

  template <class OutputIterator>
  void get_left_triangles(OutputIterator tris) const {
    get_triangles(left_, left_triangulators_, tris);
  }

  template <class OutputIterator>
  void get_right_triangles(OutputIterator tris) const {
    get_triangles(right_, right_triangulators_, tris);
  }

 private:
  struct Intersection_info {
    std::size_t left_face;
    std::size_t right_face;
    Intersection intersection;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    Intersection_info(std::size_t left_face, std::size_t right_face, Intersection&& intersection)
        : left_face(left_face), right_face(right_face), intersection(std::move(intersection)) {}

    Intersection_info(Intersection_info&& other) noexcept
        : left_face(other.left_face),
          right_face(other.right_face),
          intersection(std::move(other.intersection)) {}

    Intersection_info& operator=(Intersection_info&& other) noexcept {
      left_face = other.left_face;
      right_face = other.right_face;
      intersection = std::move(other.intersection);
      return *this;
    }
  };

  template <class OutputIterator>
  void get_triangles(const Polygon_soup<K>& soup,
                     const std::unordered_map<std::size_t, Triangulator<K>>& triangulators,
                     OutputIterator tris) const {
    for (std::size_t i = 0; i < soup.num_faces(); ++i) {
      auto it = triangulators.find(i);
      if (it == triangulators.end()) {
        *tris++ = soup.triangle(i);
      } else {
        it->second.get_triangles(tris);
      }
    }
  }

  void insert_intersection(Triangulator<K>& triangulator, const Intersection& intersection) {
    if (!intersection) {
      return;
    }

    if (const auto* p = boost::get<Point>(&*intersection)) {
      triangulator.insert(*p);
    } else if (const auto* s = boost::get<Segment>(&*intersection)) {
      auto p = s->source();
      auto q = s->target();
      auto vh0 = triangulator.insert(p);
      auto vh1 = triangulator.insert(q);
      triangulator.insert_constraint(vh0, vh1);
    } else if (const auto* t = boost::get<Triangle>(&*intersection)) {
      auto p = t->vertex(0);
      auto q = t->vertex(1);
      auto r = t->vertex(2);
      auto vh0 = triangulator.insert(p);
      auto vh1 = triangulator.insert(q);
      auto vh2 = triangulator.insert(r);
      triangulator.insert_constraint(vh0, vh1);
      triangulator.insert_constraint(vh1, vh2);
      triangulator.insert_constraint(vh2, vh0);
    } else if (const auto* points = boost::get<std::vector<Point>>(&*intersection)) {
      std::vector<typename Triangulator<K>::Vertex_handle> vhs;
      // Four to six points.
      for (const auto& p : *points) {
        vhs.push_back(triangulator.insert(p));
      }
      for (std::size_t i = 0; i < vhs.size(); ++i) {
        triangulator.insert_constraint(vhs.at(i), vhs.at((i + 1) % vhs.size()));
      }
    }
  }

  const Polygon_soup<K>& left_;
  std::unordered_map<std::size_t, Triangulator<K>> left_triangulators_;
  const Polygon_soup<K>& right_;
  std::unordered_map<std::size_t, Triangulator<K>> right_triangulators_;
};

}  // namespace kigumi

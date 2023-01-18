#pragma once

#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>

namespace kigumi {

template <class K>
class Point_projector {
  using Point_2 = typename K::Point_2;
  using Point_3 = typename K::Point_3;
  using Triangle_3 = typename K::Triangle_3;

  enum class Projection_type {
    XY,
    XZ,
    YX,
    YZ,
    ZX,
    ZY,
  };

 public:
  explicit Point_projector(const Triangle_3& triangle) {
    auto p = triangle.vertex(0);
    auto q = triangle.vertex(1);
    auto r = triangle.vertex(2);
    auto n = CGAL::normal(p, q, r);
    auto abs_nx = CGAL::abs(n.x());
    auto abs_ny = CGAL::abs(n.y());
    auto abs_nz = CGAL::abs(n.z());
    if (abs_nx >= abs_ny && abs_nx >= abs_nz) {
      projection_type_ = n.x() >= 0 ? Projection_type::YZ : Projection_type::ZY;
    } else if (abs_ny >= abs_nx && abs_ny >= abs_nz) {
      projection_type_ = n.y() >= 0 ? Projection_type::ZX : Projection_type::XZ;
    } else {
      projection_type_ = n.z() >= 0 ? Projection_type::XY : Projection_type::YX;
    }
  }

  Point_2 operator()(const Point_3& p) const {
    switch (projection_type_) {
      case Projection_type::XY:
        return {p.x(), p.y()};
      case Projection_type::XZ:
        return {p.x(), p.z()};
      case Projection_type::YX:
        return {p.y(), p.x()};
      case Projection_type::YZ:
        return {p.y(), p.z()};
      case Projection_type::ZX:
        return {p.z(), p.x()};
      case Projection_type::ZY:
        return {p.z(), p.y()};
      default:
        return CGAL::ORIGIN;
    }
  }

 private:
  Projection_type projection_type_;
};

template <class K>
class Triangulator {
  using Point_3 = typename K::Point_3;
  using Triangle_3 = typename K::Triangle_3;
  using Vb = CGAL::Triangulation_vertex_base_with_info_2<Point_3, K>;
  using Fb = CGAL::Constrained_triangulation_face_base_2<K>;
  using Tds = CGAL::Triangulation_data_structure_2<Vb, Fb>;
  using CDT = CGAL::Constrained_Delaunay_triangulation_2<K, Tds>;

 public:
  using Vertex_handle = typename Tds::Vertex_handle;
  using Intersection_of_constraints_exception = typename CDT::Intersection_of_constraints_exception;

  explicit Triangulator(const Triangle_3& triangle) : projector_(triangle) {
    auto p = triangle.vertex(0);
    auto q = triangle.vertex(1);
    auto r = triangle.vertex(2);
    insert(p);
    insert(q);
    insert(r);
  }

  template <class OutputIterator>
  void get_triangles(OutputIterator tris) const {
    for (auto it = cdt_.finite_faces_begin(); it != cdt_.finite_faces_end(); ++it) {
      const auto& p = it->vertex(0)->info();
      const auto& q = it->vertex(1)->info();
      const auto& r = it->vertex(2)->info();
      *tris++ = {p, q, r};
    }
  }

  Vertex_handle insert(const Point_3& p) {
    auto vh = cdt_.insert(projector_(p));
    vh->info() = p;
    return vh;
  }

  void insert_constraint(Vertex_handle vh_i, Vertex_handle vh_j) {
    cdt_.insert_constraint(vh_i, vh_j);
  }

  bool is_valid() const { return cdt_.is_valid(); }

 private:
  CDT cdt_;
  Point_projector<K> projector_;
};

}  // namespace kigumi

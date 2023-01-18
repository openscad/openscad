#pragma once

#include <kigumi/Mixed_mesh.h>
#include <kigumi/Operator.h>
#include <kigumi/Polygon_soup.h>

#include <array>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kigumi {

template <class K>
Polygon_soup<K> extract(const Mixed_mesh<K>& m, Operator op) {
  std::vector<typename K::Point_3> points;
  std::vector<std::array<std::size_t, 3>> faces;
  std::unordered_map<Vertex_handle, std::size_t> map;

  auto u_mask = union_mask(op);
  auto i_mask = intersection_mask(op);
  auto c_mask = coplanar_mask(op, true);
  auto o_mask = opposite_mask(op, true);

  for (auto fh : m.faces()) {
    auto mask = Mask::None;
    switch (m.data(fh).tag) {
      case Face_tag::Union:
        mask = u_mask;
        break;
      case Face_tag::Intersection:
        mask = i_mask;
        break;
      case Face_tag::Coplanar:
        mask = c_mask;
        break;
      case Face_tag::Opposite:
        mask = o_mask;
        break;
      case Face_tag::Unknown:
        break;
    }

    auto output_id = m.data(fh).from_left ? (mask & Mask::A) != Mask::None  //
                                          : (mask & Mask::B) != Mask::None;
    auto output_inv = m.data(fh).from_left ? (mask & Mask::AInv) != Mask::None  //
                                           : (mask & Mask::BInv) != Mask::None;
    if (!output_id && !output_inv) {
      continue;
    }

    const auto& f = m.face(fh);
    for (auto vh : f) {
      const auto& p = m.point(vh);
      if (!map.contains(vh)) {
        map.emplace(vh, points.size());
        points.push_back(p);
      }
    }
    if (output_inv) {
      faces.push_back({map.at(f[0]), map.at(f[2]), map.at(f[1])});
    } else {
      faces.push_back({map.at(f[0]), map.at(f[1]), map.at(f[2])});
    }
  }

  return {std::move(points), std::move(faces)};
}

}  // namespace kigumi

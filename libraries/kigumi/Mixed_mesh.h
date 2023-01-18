#pragma once

#include <kigumi/Mesh.h>

namespace kigumi {

enum class Face_tag { Union, Intersection, Coplanar, Opposite, Unknown };

struct Face_data {
  bool from_left = false;
  Face_tag tag = Face_tag::Unknown;
};

template <class K>
using Mixed_mesh = Mesh<K, Face_data>;

}  // namespace kigumi

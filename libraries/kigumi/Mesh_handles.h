#pragma once

#include <functional>
#include <limits>

namespace kigumi {

struct Face_handle {
  std::size_t i = std::numeric_limits<std::size_t>::max();
};

inline bool operator<(Face_handle a, Face_handle b) { return a.i < b.i; }
inline bool operator==(Face_handle a, Face_handle b) { return a.i == b.i; }
inline bool operator!=(Face_handle a, Face_handle b) { return a.i != b.i; }

struct Vertex_handle {
  std::size_t i = std::numeric_limits<std::size_t>::max();
};

inline bool operator<(Vertex_handle a, Vertex_handle b) { return a.i < b.i; }
inline bool operator==(Vertex_handle a, Vertex_handle b) { return a.i == b.i; }
inline bool operator!=(Vertex_handle a, Vertex_handle b) { return a.i != b.i; }

}  // namespace kigumi

template <>
struct std::hash<kigumi::Vertex_handle> {
  std::size_t operator()(kigumi::Vertex_handle vh) const { return vh.i; }
};

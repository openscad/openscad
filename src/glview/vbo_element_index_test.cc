// Vertex deduplication for indexed preview buffers. uniqueElementIndex() runs for every vertex of
// every triangle whenever a preview's VBOs are built; profiling octopus.scad put ~2.5 s of a 3.2 s
// build inside this lookup, which hashed each vertex byte by byte twice (find, then emplace).

#include <catch2/catch_all.hpp>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "glview/VBOBuilder.h"

namespace {

// A plausible interleaved vertex: position, normal, color and barycentric floats.
std::vector<GLbyte> vertexBytes(size_t id)
{
  std::vector<GLbyte> bytes(13 * sizeof(float));
  for (size_t i = 0; i < 13; ++i) {
    const float value = static_cast<float>(id) + static_cast<float>(i) * 0.25f;
    std::memcpy(bytes.data() + i * sizeof(float), &value, sizeof(float));
  }
  return bytes;
}

}  // namespace

TEST_CASE("uniqueElementIndex reuses indices for identical vertices", "[glview][VBOBuilder]")
{
  ElementsMap map;
  const auto first = uniqueElementIndex(map, vertexBytes(7));
  CHECK(first == std::make_pair(GLuint{0}, true));
  CHECK(uniqueElementIndex(map, vertexBytes(8)) == std::make_pair(GLuint{1}, true));
  CHECK(uniqueElementIndex(map, vertexBytes(7)) == std::make_pair(GLuint{0}, false));
  CHECK(map.size() == 2);
}

namespace {

size_t hashes = 0;

struct CountingHash {
  size_t operator()(const std::vector<GLbyte>& vertex) const
  {
    ++hashes;
    return vertex_hash<std::vector<GLbyte>>{}(vertex);
  }
};

}  // namespace

TEST_CASE("uniqueElementIndex hashes each vertex once", "[glview][VBOBuilder]")
{
  // A wall-clock limit would depend on the machine; the doubled hash it would catch does not. A
  // find() followed by emplace() hashes every new vertex twice, which on octopus.scad was a third of
  // the whole buffer build.
  std::unordered_map<std::vector<GLbyte>, GLuint, CountingHash> map;
  map.reserve(64);  // no rehash, whose extra hashing would be counted too
  hashes = 0;
  size_t lookups = 0;
  for (int round = 0; round < 3; ++round) {
    for (size_t id = 0; id < 10; ++id) {
      uniqueElementIndex(map, vertexBytes(id));
      ++lookups;
    }
  }
  REQUIRE(map.size() == 10);
  CHECK(hashes == lookups);
}

// Round-trip tests for the compute worker's binary geometry transport (feature 32).
//
// This is an internal IPC format, not a user-facing file format: both ends are the same
// build of OpenSCAD on the same machine, which is what makes raw native doubles safe here
// and unsafe in anything a user could keep.
#include "io/ipc_geometry.h"

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "geometry/PolySet.h"

namespace {

std::string tempPath(const std::string& name)
{
  return (std::filesystem::temp_directory_path() / ("openscad-ipc-test-" + name)).string();
}

// The production writer takes an ostream, like every other exporter; the tests want a file,
// because the reader takes a path and a truncation test needs something to truncate.
bool writeToFile(const PolySet& polyset, const std::string& path)
{
  std::ofstream stream(path, std::ios::binary);
  export_ipc_geometry(polyset, stream);
  stream.flush();
  stream.close();
  return stream.good();
}

// Deliberately not all triangles: PolySet holds arbitrary n-gons, and the transport has to
// survive them. A format that quietly assumed triangles would pass a triangle-only test and
// then drop faces on the first quad a real preview produced.
std::shared_ptr<PolySet> sampleMesh()
{
  auto ps = std::make_shared<PolySet>(3);
  ps->vertices = {{0, 0, 0},     {1, 0, 0},        {1, 1, 0},         {0, 1, 0},
                  {0.5, 0.5, 1}, {-1.25e-9, 2, 3}, {1e17, -1e17, 0.1}};
  ps->indices = {{0, 1, 2, 3}, {0, 1, 4}, {1, 2, 4}, {5, 6, 4, 0, 1}};
  ps->setConvexity(7);
  return ps;
}

}  // namespace

TEST_CASE("binary IPC geometry survives a round trip", "[ipc][geometry]")
{
  const auto mesh = sampleMesh();
  const auto path = tempPath("roundtrip.bin");
  REQUIRE(writeToFile(*mesh, path));

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  // Bit-exact, not approximately equal: the whole point of the binary format is that the
  // GUI sees the doubles the worker computed, with no decimal detour.
  REQUIRE(bool(read->vertices == mesh->vertices));
  REQUIRE(bool(read->indices == mesh->indices));
  REQUIRE(read->getConvexity() == mesh->getConvexity());
  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry carries per-face colors", "[ipc][geometry]")
{
  auto mesh = sampleMesh();
  mesh->colors = {Color4f(1.0f, 0.0f, 0.0f, 1.0f), Color4f(0.25f, 0.5f, 0.75f, 0.125f)};
  mesh->color_indices = {0, -1, 1, 0};
  const auto path = tempPath("colors.bin");
  REQUIRE(writeToFile(*mesh, path));

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  REQUIRE(read->color_indices == mesh->color_indices);
  REQUIRE(read->colors.size() == mesh->colors.size());
  // The ASCII OFF transport quantizes colors to 8 bits per channel, so 0.125 alpha does not
  // survive it. Binary must not lose that.
  REQUIRE(bool(read->colors == mesh->colors));
  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry round-trips an empty mesh", "[ipc][geometry]")
{
  const PolySet empty(3);
  const auto path = tempPath("empty.bin");
  REQUIRE(writeToFile(empty, path));
  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  REQUIRE(read->vertices.empty());
  REQUIRE(read->indices.empty());
  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry rejects damaged payloads", "[ipc][geometry]")
{
  const auto mesh = sampleMesh();
  const auto path = tempPath("damaged.bin");
  REQUIRE(writeToFile(*mesh, path));
  const auto size = std::filesystem::file_size(path);

  SECTION("truncated")
  {
    // A worker killed mid-write is the normal case here, not an exotic one: feature 31 hard-kills
    // the worker on Stop. A short read must fail, not hand back a half mesh.
    std::filesystem::resize_file(path, size / 2);
    REQUIRE(!import_ipc_geometry(path));
  }

  SECTION("wrong magic")
  {
    std::fstream stream(path, std::ios::in | std::ios::out | std::ios::binary);
    stream.seekp(0);
    stream.write("XXXX", 4);
    stream.close();
    REQUIRE(!import_ipc_geometry(path));
  }

  SECTION("absent")
  {
    std::filesystem::remove(path);
    REQUIRE(!import_ipc_geometry(path));
  }

  std::filesystem::remove(path);
}

// The payload's layout is only ever exercised by a same-machine round trip, which cannot fail
// on byte order (both ends share it) and cannot fail on padding (both ends share the struct).
// These tests pin the layout to fixed bytes instead, so CI catches the changes that a round
// trip is structurally blind to:
//
//   * a field added, reordered, or resized in Header;
//   * padding introduced by changing a member's type;
//   * a stream opened in text mode, which on Windows rewrites every 0x0A byte in the payload
//     to 0x0D 0x0A and corrupts any double or index containing that byte.
//
// Byte order itself is asserted rather than handled: the format is documented same-machine, and
// every platform this project builds on is little-endian. If OpenSCAD is ever ported to a
// big-endian target, this test is the thing that should fail and force the decision.
namespace {

bool isLittleEndian()
{
  const uint32_t one = 1;
  return *reinterpret_cast<const unsigned char *>(&one) == 1;
}

}  // namespace

TEST_CASE("binary IPC geometry has a fixed on-disk layout", "[ipc][geometry]")
{
  REQUIRE(isLittleEndian());

  PolySet mesh(3);
  mesh.vertices = {{1.0, 2.0, 3.0}};
  mesh.indices = {{0}};
  mesh.setConvexity(1);

  std::ostringstream stream(std::ios::binary);
  export_ipc_geometry(mesh, stream);
  const auto bytes = stream.str();

  // 40-byte header + one vertex (3 doubles) + one polygon (count + one index).
  REQUIRE(bytes.size() == 40 + 24 + 8);

  const std::vector<unsigned char> expected{
    0x4f, 0x53, 0x49, 0x47,                          // magic "OSIG"
    0x01, 0x00, 0x00, 0x00,                          // version 1
    0x03, 0x00, 0x00, 0x00,                          // dimension 3
    0x01, 0x00, 0x00, 0x00,                          // convexity 1
    0x00, 0x00, 0x00, 0x00,                          // flags: not triangular, not manifold
    0x01, 0x00, 0x00, 0x00,                          // 1 vertex
    0x01, 0x00, 0x00, 0x00,                          // 1 polygon
    0x01, 0x00, 0x00, 0x00,                          // 1 index in total
    0x00, 0x00, 0x00, 0x00,                          // no colors
    0x00, 0x00, 0x00, 0x00,                          // no color indices
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,  // 1.0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,  // 2.0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x40,  // 3.0
    0x01, 0x00, 0x00, 0x00,                          // polygon vertex count
    0x00, 0x00, 0x00, 0x00,                          // index 0
  };
  REQUIRE(bytes.size() == expected.size());
  REQUIRE(std::equal(bytes.begin(), bytes.end(), expected.begin(), [](char actual, unsigned char want) {
    return static_cast<unsigned char>(actual) == want;
  }));
}

TEST_CASE("binary IPC geometry is not mangled by text-mode translation", "[ipc][geometry]")
{
  // 0x0A appears inside ordinary doubles, so a stream opened without std::ios::binary silently
  // grows the payload on Windows and every later field decodes as garbage. 3.25 and 2053.0 both
  // encode a literal newline byte, which makes that failure deterministic rather than lucky:
  //   3.25   -> 00 00 00 00 00 00 0a 40
  //   2053.0 -> 00 00 00 00 00 0a a0 40
  PolySet mesh(3);
  mesh.vertices = {{3.25, 2053.0, 0.0}};
  mesh.indices = {{0}};
  const auto path = tempPath("newline.bin");
  REQUIRE(writeToFile(mesh, path));

  const auto onDisk = std::filesystem::file_size(path);
  REQUIRE(onDisk == 40 + 24 + 8);

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  REQUIRE(bool(read->vertices == mesh.vertices));
  std::filesystem::remove(path);
}

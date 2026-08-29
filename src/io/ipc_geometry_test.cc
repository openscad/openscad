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

#include "geometry/Geometry.h"
#include "geometry/Polygon2d.h"
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

  const auto read = import_ipc_polyset(path);
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

  const auto read = import_ipc_polyset(path);
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
  const auto read = import_ipc_polyset(path);
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
    REQUIRE(!import_ipc_polyset(path));
  }

  SECTION("wrong magic")
  {
    std::fstream stream(path, std::ios::in | std::ios::out | std::ios::binary);
    stream.seekp(0);
    stream.write("XXXX", 4);
    stream.close();
    REQUIRE(!import_ipc_polyset(path));
  }

  SECTION("absent")
  {
    std::filesystem::remove(path);
    REQUIRE(!import_ipc_polyset(path));
  }

  std::filesystem::remove(path);
}

// A same-machine round trip cannot catch a stream opened in text mode: on Windows that rewrites
// every 0x0A byte in the payload as 0x0D 0x0A, and ordinary doubles are full of them. The file
// then no longer matches the length its own header implies, and every field after the first
// newline byte decodes as garbage.
//
// Byte order and struct layout are deliberately NOT pinned here. The writer and reader are the
// same executable on the same machine -- feature 29 launches the worker from
// QCoreApplication::applicationFilePath() -- so a reordered field or new padding changes both
// ends at once and cannot produce a mismatch. A golden-bytes fixture would only make every
// legitimate format change require editing the fixture.
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

  // 16 list header + 40 body header + 24 vertex + 8 one-index polygon. The exact size is the
  // assertion with teeth: text-mode translation shows up as extra bytes, not as a bad decode.
  const auto onDisk = std::filesystem::file_size(path);
  REQUIRE(onDisk == 16 + 40 + 24 + 8);

  const auto read = import_ipc_polyset(path);
  REQUIRE(read);
  REQUIRE(bool(read->vertices == mesh.vertices));
  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry preserves separate bodies", "[ipc][geometry]")
{
  // A rendered model can be a GeometryList of separate bodies. Flattening it to one mesh in
  // transit is invisible in the preview but destroys anything downstream that works per body --
  // multi-file export sees a single body and offers nothing to separate.
  auto first = std::make_shared<PolySet>(3);
  first->vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  first->indices = {{0, 1, 2}};
  auto second = std::make_shared<PolySet>(3);
  second->vertices = {{5, 0, 0}, {6, 0, 0}, {5, 1, 0}, {6, 1, 0}};
  second->indices = {{0, 1, 3, 2}};

  Geometry::Geometries bodies;
  bodies.emplace_back(nullptr, first);
  bodies.emplace_back(nullptr, second);
  const std::shared_ptr<const Geometry> list = std::make_shared<GeometryList>(bodies);

  const auto path = tempPath("bodies.bin");
  {
    std::ofstream stream(path, std::ios::binary);
    export_ipc_geometry(list, stream);
  }

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  const auto readList = std::dynamic_pointer_cast<const GeometryList>(read);
  REQUIRE(readList);
  REQUIRE(readList->getChildren().size() == 2);

  auto it = readList->getChildren().begin();
  const auto a = std::dynamic_pointer_cast<const PolySet>(it->second);
  const auto b = std::dynamic_pointer_cast<const PolySet>((++it)->second);
  REQUIRE(a);
  REQUIRE(b);
  REQUIRE(bool(a->vertices == first->vertices));
  REQUIRE(bool(b->vertices == second->vertices));
  REQUIRE(bool(b->indices == second->indices));

  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry keeps a single body unwrapped", "[ipc][geometry]")
{
  // One body must not become a one-element list: everything downstream treats a bare PolySet
  // as "one body", and wrapping it would change behaviour for every ordinary model.
  const auto mesh = sampleMesh();
  const auto path = tempPath("single.bin");
  REQUIRE(writeToFile(*mesh, path));

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  REQUIRE(std::dynamic_pointer_cast<const PolySet>(read));
  REQUIRE_FALSE(std::dynamic_pointer_cast<const GeometryList>(read));

  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry carries opaque per-body metadata", "[ipc][geometry]")
{
  // The point of the channel: the transport stores and returns these pairs without knowing what
  // they mean, so a feature can annotate a body without any change here. Materials is the first
  // caller, but nothing in the transport mentions materials.
  auto first = std::make_shared<PolySet>(3);
  first->vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  first->indices = {{0, 1, 2}};
  first->setMetadata("material", "PLA");
  first->setMetadata("anything", "at all");
  auto second = std::make_shared<PolySet>(3);
  second->vertices = {{5, 0, 0}, {6, 0, 0}, {5, 1, 0}};
  second->indices = {{0, 1, 2}};
  second->setMetadata("material", "PETG");

  Geometry::Geometries bodies;
  bodies.emplace_back(nullptr, first);
  bodies.emplace_back(nullptr, second);
  const std::shared_ptr<const Geometry> list = std::make_shared<GeometryList>(bodies);

  const auto path = tempPath("metadata.bin");
  {
    std::ofstream stream(path, std::ios::binary);
    export_ipc_geometry(list, stream);
  }

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  const auto readList = std::dynamic_pointer_cast<const GeometryList>(read);
  REQUIRE(readList);
  REQUIRE(readList->getChildren().size() == 2);
  auto it = readList->getChildren().begin();
  REQUIRE(it->second->getMetadata("material") == "PLA");
  REQUIRE(it->second->getMetadata("anything") == "at all");
  REQUIRE((++it)->second->getMetadata("material") == "PETG");
  // An absent key is empty, not a crash.
  REQUIRE(it->second->getMetadata("absent").empty());

  std::filesystem::remove(path);
}

TEST_CASE("binary IPC geometry round-trips a 2D body", "[ipc][geometry]")
{
  // A 2D result used to be forced through the mesh path, which cannot represent outlines.
  auto polygon = std::make_shared<Polygon2d>();
  Outline2d outer;
  outer.vertices = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
  polygon->addOutline(outer);
  Outline2d hole;
  hole.vertices = {{2, 2}, {4, 2}, {4, 4}};
  hole.positive = false;
  polygon->addOutline(hole);
  polygon->setSanitized(true);
  polygon->setMetadata("material", "PLA");

  const auto path = tempPath("polygon2d.bin");
  {
    std::ofstream stream(path, std::ios::binary);
    export_ipc_geometry(std::shared_ptr<const Geometry>(polygon), stream);
  }

  const auto read = import_ipc_geometry(path);
  REQUIRE(read);
  const auto readPolygon = std::dynamic_pointer_cast<const Polygon2d>(read);
  REQUIRE(readPolygon);
  REQUIRE(readPolygon->outlines().size() == 2);
  REQUIRE(readPolygon->isSanitized());
  REQUIRE(readPolygon->getDimension() == 2);
  REQUIRE(bool(readPolygon->outlines()[0].vertices == outer.vertices));
  REQUIRE_FALSE(readPolygon->outlines()[1].positive);
  REQUIRE(readPolygon->getMetadata("material") == "PLA");

  std::filesystem::remove(path);
}

#include "io/import.h"

#include <catch2/catch_all.hpp>
#include <memory>
#include <string>

#include "core/AST.h"
#include "geometry/PolySet.h"

static std::string get_obj_file_path(const std::string& name)
{
  return std::string(OPENSCAD_TEST_DATA_DIR) + "/obj/" + name;
}

static const Location test_location{0, 0, 0, 0, nullptr};

static void check_tetrahedron(const std::string& name)
{
  auto ps = import_obj(get_obj_file_path(name), test_location);
  REQUIRE(ps);
  REQUIRE_FALSE(ps->isEmpty());
  CHECK(ps->vertices.size() == 4);
  REQUIRE(ps->indices.size() == 4);
  for (const auto& face : ps->indices) {
    CHECK(face.size() == 3);
    for (const auto idx : face) {
      CHECK(idx < ps->vertices.size());
    }
  }
}

TEST_CASE("OBJ importer handles malformed files without crashing", "[io][OBJ-Import]")
{
  SECTION("non-numeric face index is rejected")
  {
    auto ps = import_obj(get_obj_file_path("bogus-face-index.obj"), test_location);
    REQUIRE(ps);
    CHECK(ps->isEmpty());
  }

  SECTION("face index that doesn't fit an int is rejected")
  {
    auto ps = import_obj(get_obj_file_path("huge-face-index.obj"), test_location);
    REQUIRE(ps);
    CHECK(ps->isEmpty());
  }
}

TEST_CASE("OBJ importer reads common variations of the format", "[io][OBJ-Import]")
{
  SECTION("plain file")
  {
    check_tetrahedron("valid-tetra.obj");
  }

  SECTION("multiple separators between face indices")
  {
    check_tetrahedron("double-space-face.obj");
  }

  SECTION("vertex colors are ignored")
  {
    check_tetrahedron("vertex-colors.obj");
  }

  SECTION("negative face indices are relative")
  {
    check_tetrahedron("negative-face-index.obj");
  }
}

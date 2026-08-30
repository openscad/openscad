#include "io/import.h"

#include <catch2/catch_all.hpp>
#include <memory>
#include <string>

#include "core/AST.h"
#include "geometry/PolySet.h"

static std::string get_off_file_path(const std::string& name)
{
  return std::string(OPENSCAD_TEST_DATA_DIR) + "/off/" + name;
}

static const Location test_location{0, 0, 0, 0, nullptr};

TEST_CASE("OFF importer handles malformed files without crashing", "[io][OFF-Import]")
{
  SECTION("face indices beyond parsed vertex count are rejected, file with trailing newline")
  {
    // header declares 1000 vertices but the file is truncated
    auto ps = import_off(get_off_file_path("bogus-face-index.off"), test_location);
    REQUIRE(ps);
    CHECK(ps->isEmpty());
    CHECK(ps->vertices.empty());
  }

  SECTION("face indices beyond parsed vertex count are rejected, file with no trailing newline")
  {
    // header declares 1000 vertices but the file is truncated
    auto ps = import_off(get_off_file_path("bogus-face-index-no-nl.off"), test_location);
    REQUIRE(ps);
    CHECK(ps->isEmpty());
    CHECK(ps->vertices.size() == 4);  // Current state; improved validation should fix this.
  }

  SECTION("excessive header element counts are rejected without abort")
  {
    auto ps = import_off(get_off_file_path("huge-counts.off"), test_location);
    REQUIRE(ps);
    CHECK(ps->isEmpty());
  }

  SECTION("well-formed file is imported intact")
  {
    auto ps = import_off(get_off_file_path("valid-tetra.off"), test_location);
    REQUIRE(ps);
    REQUIRE_FALSE(ps->isEmpty());
    CHECK(ps->vertices.size() == 4);
    REQUIRE(ps->indices.size() == 2);
    for (const auto& face : ps->indices) {
      CHECK(face.size() == 3);
      for (const auto idx : face) {
        CHECK(idx < ps->vertices.size());
      }
    }
  }
}

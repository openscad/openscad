// Benchmark for feature 32: where does compute-worker IPC time actually go?
//
// The worker hands preview/render geometry to the GUI through a temporary file holding
// full-precision ASCII OFF. This measures the four phases separately so the replacement
// transport is chosen from numbers instead of intuition:
//
//   serialize  ASCII formatting of doubles (export_off into a memory stream)
//   write      putting those bytes on the filesystem, flushed and closed
//   read       pulling the same bytes back (page cache, in practice)
//   parse      import_off on the file, minus the read cost
//   binary     memcpy round trip of the same mesh through a flat buffer
//
// Hidden by default ("[.]"); run explicitly:
//   ./OpenSCADUnitTests "[ipc-bench]" -s
#include <catch2/catch_all.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "geometry/PolySet.h"
#include "io/export.h"
#include "io/import.h"
#include "io/ipc_geometry.h"

namespace {

// A closed-ish strip of triangles; content does not matter, size and float width do.
std::shared_ptr<PolySet> makeMesh(size_t triangles)
{
  auto ps = std::make_shared<PolySet>(3);
  ps->setTriangular(true);
  ps->vertices.reserve(triangles + 2);
  for (size_t i = 0; i < triangles + 2; ++i) {
    // Irrational-ish coordinates so every double needs its full 17 digits, which is what
    // the real export writes and the real import parses.
    const double t = static_cast<double>(i) * 0.7071067811865476;
    ps->vertices.emplace_back(t, t * 1.4142135623730951, t * 0.3333333333333333);
  }
  ps->indices.reserve(triangles);
  for (size_t i = 0; i < triangles; ++i) {
    ps->indices.push_back({static_cast<int>(i), static_cast<int>(i + 1), static_cast<int>(i + 2)});
  }
  return ps;
}

using Clock = std::chrono::steady_clock;
double msSince(const Clock::time_point& start)
{
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// The flat binary layout feature 32 proposes: header, vertices, then triangle indices.
std::vector<char> toBinary(const PolySet& ps)
{
  const uint32_t vertexCount = ps.vertices.size();
  const uint32_t indexCount = ps.indices.size() * 3;
  std::vector<char> buffer(8 + vertexCount * 3 * sizeof(double) + indexCount * sizeof(uint32_t));
  auto *out = buffer.data();
  std::memcpy(out, &vertexCount, 4);
  std::memcpy(out + 4, &indexCount, 4);
  out += 8;
  for (const auto& v : ps.vertices) {
    const double xyz[3]{v.x(), v.y(), v.z()};
    std::memcpy(out, xyz, sizeof(xyz));
    out += sizeof(xyz);
  }
  for (const auto& face : ps.indices) {
    for (const auto index : face) {
      const uint32_t value = index;
      std::memcpy(out, &value, 4);
      out += 4;
    }
  }
  return buffer;
}

std::shared_ptr<PolySet> fromBinary(const std::vector<char>& buffer)
{
  auto ps = std::make_shared<PolySet>(3);
  uint32_t vertexCount = 0, indexCount = 0;
  std::memcpy(&vertexCount, buffer.data(), 4);
  std::memcpy(&indexCount, buffer.data() + 4, 4);
  const auto *in = buffer.data() + 8;
  ps->vertices.resize(vertexCount);
  std::memcpy(ps->vertices.data(), in, vertexCount * 3 * sizeof(double));
  in += vertexCount * 3 * sizeof(double);
  ps->indices.resize(indexCount / 3);
  for (auto& face : ps->indices) {
    face.resize(3);
    for (auto& index : face) {
      uint32_t value = 0;
      std::memcpy(&value, in, 4);
      in += 4;
      index = value;
    }
  }
  return ps;
}

// One model's worth of transport timings, reported the same way for synthetic and real meshes.
void measure(const std::string& label, const std::shared_ptr<const PolySet>& mesh,
             const std::string& path)
{
  {
    auto start = Clock::now();
    std::ostringstream ascii;
    ascii << std::setprecision(std::numeric_limits<double>::max_digits10);
    export_off(mesh, ascii);
    const auto serializeMs = msSince(start);
    const auto text = ascii.str();

    start = Clock::now();
    {
      std::ofstream stream(path, std::ios::binary);
      stream << text;
      stream.flush();
      stream.close();
    }
    const auto writeMs = msSince(start);

    start = Clock::now();
    size_t bytesRead = 0;
    {
      // Block read, not istreambuf_iterator: the point is to measure moving the bytes, and a
      // character-at-a-time iterator measures the iterator instead. (First cut of this
      // benchmark did that and reported the read as 200x the write.)
      std::ifstream stream(path, std::ios::binary);
      std::string back(text.size(), '\0');
      stream.read(back.data(), back.size());
      bytesRead = stream.gcount();
    }
    const auto readMs = msSince(start);
    REQUIRE(bytesRead == text.size());

    start = Clock::now();
    const auto imported = import_off(path, Location::NONE);
    const auto importMs = msSince(start);
    REQUIRE(imported);
    REQUIRE(imported->numFacets() == mesh->numFacets());

    start = Clock::now();
    const auto binary = toBinary(*mesh);
    const auto binaryOutMs = msSince(start);
    start = Clock::now();
    const auto decoded = fromBinary(binary);
    const auto binaryInMs = msSince(start);

    // Binary must be bit-exact; ASCII at max_digits10 is expected to be too, and if it ever
    // is not, that is a fidelity argument for binary on top of the timing one.
    // Compared as a bool, not as the containers themselves: a failing container REQUIRE
    // expands both sides into the report, which for a million triangles is unreadable.
    REQUIRE(decoded->vertices.size() == mesh->vertices.size());
    REQUIRE(decoded->indices.size() == mesh->indices.size());
    REQUIRE(bool(decoded->vertices == mesh->vertices));
    REQUIRE(bool(decoded->indices == mesh->indices));

    WARN(label << " | " << mesh->numFacets() << " facets, " << mesh->vertices.size()
               << " vertices | ascii " << text.size() / 1024 << " KiB, binary " << binary.size() / 1024
               << " KiB\n"
               << "  serialize " << serializeMs << " ms, write " << writeMs << " ms, read " << readMs
               << " ms, import(read+parse) " << importMs << " ms\n"
               << "  binary encode " << binaryOutMs << " ms, decode " << binaryInMs << " ms");
  }
}

}  // namespace

TEST_CASE("compute-worker IPC transport cost breakdown", "[.][ipc-bench]")
{
  for (const size_t triangles : {size_t{5000}, size_t{200000}, size_t{1000000}}) {
    measure(std::to_string(triangles) + " synthetic triangles", makeMesh(triangles),
            (std::filesystem::temp_directory_path() /
             ("openscad-ipc-bench-" + std::to_string(triangles) + ".off"))
              .string());
  }
}

// Real models, rendered to OFF beforehand by the CLI. Synthetic meshes deliberately use
// irrational coordinates, which is the worst case for ASCII width; real models mix in round
// numbers, so this is the check that the conclusion survives realistic data.
//
//   OPENSCAD_IPC_BENCH_OFF=/path/a.off:/path/b.off ./OpenSCADUnitTests "[ipc-bench-real]"
TEST_CASE("compute-worker IPC transport cost on real models", "[.][ipc-bench-real]")
{
  const auto *const paths = std::getenv("OPENSCAD_IPC_BENCH_OFF");
  if (!paths || !*paths) {
    WARN("Set OPENSCAD_IPC_BENCH_OFF to a ':'-separated list of pre-rendered .off files.");
    return;
  }
  std::string list(paths);
  size_t start = 0;
  while (start <= list.size()) {
    const auto end = std::min(list.find(':', start), list.size());
    const auto path = list.substr(start, end - start);
    start = end + 1;
    if (path.empty()) continue;
    // Time whichever importer the payload actually calls for, so a baseline (.off) build and
    // a binary build can both be pointed at their own worker output and compared directly.
    std::ifstream probe(path, std::ios::binary);
    char magic[4]{};
    probe.read(magic, sizeof(magic));
    probe.close();
    const bool binaryPayload = std::string(magic, 4) == "OSIG";
    auto start = Clock::now();
    auto mesh = binaryPayload ? import_ipc_polyset(path) : import_off(path, Location::NONE);
    WARN(std::filesystem::path(path).filename().string()
         << " | decode as " << (binaryPayload ? "binary" : "ASCII OFF") << ": " << msSince(start)
         << " ms");
    REQUIRE(mesh);
    measure(std::filesystem::path(path).filename().string(), std::move(mesh),
            path + ".ipc-bench-roundtrip.off");
  }
}

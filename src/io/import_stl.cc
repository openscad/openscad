#include "io/import.h"

#include <boost/predef.h>

#include <algorithm>
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/AST.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "utils/printutils.h"

#if !defined(BOOST_ENDIAN_BIG_BYTE_AVAILABLE) && !defined(BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE)
#error Byte order undefined or unknown. Currently only BOOST_ENDIAN_BIG_BYTE and BOOST_ENDIAN_LITTLE_BYTE are supported.
#endif

inline constexpr size_t STL_FACET_NUMBYTES = 4ul * 3ul * 4ul + 2ul;
inline constexpr uint32_t STL_FACETS_PER_BATCH = 4096;
inline constexpr uint64_t STL_MAX_RESERVED_VERTICES = 1'000'000;
static_assert(sizeof(float) == 4, "STL requires 32-bit floating-point coordinates");

#if BOOST_ENDIAN_BIG_BYTE
static void uint32_byte_swap(unsigned char *p)
{
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3) || defined(__clang__)
  uint32_t& x = *reinterpret_cast<uint32_t *>(p);
  x = __builtin_bswap32(x);
#elif defined(_MSC_VER)
  uint32_t& x = *reinterpret_cast<uint32_t *>(p);
  x = _byteswap_ulong(x);
#else
  std::swap(*p, *(p + 3));
  std::swap(*(p + 1), *(p + 2));
#endif
}

static void uint32_byte_swap(uint32_t& x)
{
  uint32_byte_swap(reinterpret_cast<unsigned char *>(&x));
}
#endif  // if BOOST_ENDIAN_BIG_BYTE

static void appendBinaryFacet(PolySetBuilder& builder, const char *bytes)
{
  std::array<float, 9> coordinates;
  // Skip the stored normal. The builder needs only the triangle's vertices.
  std::memcpy(coordinates.data(), bytes + 3 * sizeof(float), coordinates.size() * sizeof(float));
#if BOOST_ENDIAN_BIG_BYTE
  for (auto& coordinate : coordinates) {
    auto *coordinateBytes = reinterpret_cast<unsigned char *>(&coordinate);
    std::reverse(coordinateBytes, coordinateBytes + sizeof(coordinate));
  }
#endif
  builder.appendTriangle(Vector3d(coordinates[0], coordinates[1], coordinates[2]),
                         Vector3d(coordinates[3], coordinates[4], coordinates[5]),
                         Vector3d(coordinates[6], coordinates[7], coordinates[8]));
}

std::unique_ptr<PolySet> import_stl(const std::string& filename, const Location& loc)
{
  // Open file and position at the end
  std::ifstream f(std::filesystem::u8path(filename), std::ios::in | std::ios::binary | std::ios::ate);
  if (!f.good()) {
    LOG(message_group::Warning, "Can't open import file '%1$s', import() at line %2$d", filename,
        loc.firstLine());
    return PolySet::createEmpty();
  }

  uint32_t facenum = 0;
  const boost::regex ex_sfe(R"(^\s*solid|^\s*facet|^\s*endfacet)");
  const boost::regex ex_outer("^\\s*outer loop$");
  const boost::regex ex_loopend("^\\s*endloop$");
  const boost::regex ex_vertex("^\\s*vertex");
  const boost::regex ex_vertices(R"(^\s*vertex\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)\s*$)");
  const boost::regex ex_endsolid("^\\s*endsolid");

  bool binary = false;
  const std::streampos file_size = f.tellg();
  f.seekg(80);
  if (f.good() && !f.eof()) {
    f.read((char *)&facenum, sizeof(uint32_t));
#if BOOST_ENDIAN_BIG_BYTE
    uint32_byte_swap(facenum);
#endif
    if (file_size == static_cast<std::streamoff>(80ul + 4ul + 50ul * facenum)) {
      binary = true;
    }
  }
  if (!binary) facenum = 0;
  PolySetBuilder builder(0, facenum);
  f.seekg(0);

  char data[5];
  f.read(data, 5);
  if (!binary && !f.eof() && f.good() && !memcmp(data, "solid", 5)) {
    int i = 0;
    int lineno = 1;
    std::array<std::array<double, 3>, 3> vdata;
    std::string line;

    auto AsciiError = [&](const auto& errstr) {
      LOG(message_group::Error, loc, "", "STL line %1$s, %2$s line '%3$s' importing file '%4$s'", lineno,
          errstr, line, filename);
    };

    std::getline(f, line);
    bool reached_end = false;
    while (!f.eof()) {
      lineno++;
      std::getline(f, line);
      boost::trim(line);
      boost::smatch results;

      if (line.length() == 0 || boost::regex_search(line, ex_sfe)) {
        continue;
      } else if (boost::regex_search(line, ex_outer)) {
        i = 0;
        continue;
      } else if (boost::regex_search(line, ex_loopend)) {
        if (i < 3) {
          AsciiError("missing vertex");
        }
        continue;
      } else if (boost::regex_search(line, ex_endsolid)) {
        reached_end = true;
        break;
      } else if (i >= 3) {
        AsciiError("extra vertex");
        return PolySet::createEmpty();
      } else if (boost::regex_search(line, results, ex_vertices) && results.size() >= 4) {
        try {
          for (int v = 0; v < 3; ++v) {
            vdata.at(i).at(v) = boost::lexical_cast<double>(results[v + 1]);
          }
          if (++i == 3) {
            builder.beginPolygon(3);
            for (int j = 0; j < 3; j++) {
              builder.addVertex(Vector3d(vdata[j][0], vdata[j][1], vdata[j][2]));
            }
          }
        } catch (const boost::bad_lexical_cast& blc) {
          AsciiError("can't parse vertex");
          return PolySet::createEmpty();
        }
      }
    }
    if (!reached_end) {
      AsciiError("file incomplete");
    }
  } else if (binary && !f.eof() && f.good()) {
    try {
      f.ignore(80 - 5 + 4);
      const auto batchCapacity = std::min(facenum, STL_FACETS_PER_BATCH);
      std::vector<char> facets(batchCapacity * STL_FACET_NUMBYTES);
      for (auto remaining = facenum; remaining != 0;) {
        const auto count = std::min(remaining, batchCapacity);
        const auto bytes = static_cast<std::streamsize>(count * STL_FACET_NUMBYTES);
        if (!f.read(facets.data(), bytes)) {
          throw std::ios_base::failure("facet data truncated");
        }
        for (uint32_t i = 0; i < count; ++i) {
          appendBinaryFacet(builder, facets.data() + i * STL_FACET_NUMBYTES);
        }
        if (remaining == facenum && count < remaining) {
          // Estimate vertex reuse from the first batch to avoid growing the hash table repeatedly.
          const auto verticesInBatch = static_cast<uint64_t>(builder.numVertices());
          const auto estimatedVertices = verticesInBatch * facenum / count;
          builder.reserve(static_cast<int>(std::min(estimatedVertices, STL_MAX_RESERVED_VERTICES)));
        }
        remaining -= count;
      }
    } catch (const std::ios_base::failure& ex) {
      int64_t offset = -1;
      try {
        offset = f.tellg();
      } catch (...) {
      }
      if (offset < 0) {
        LOG(message_group::Error, loc, "", "Binary STL '%1$s' error: %3$s", filename, ex.what());
      } else {
        LOG(message_group::Error, loc, "", "Binary STL '%1$s' error at byte %2$s: %3$s", filename,
            offset, ex.what());
      }
      return PolySet::createEmpty();
    }
  } else {
    LOG(message_group::Error, loc, "", "STL format not recognized in '%1$s'.", filename);
    return PolySet::createEmpty();
  }
  return builder.build();
}

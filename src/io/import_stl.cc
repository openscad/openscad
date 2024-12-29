#include "io/import.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "utils/printutils.h"
#include "core/AST.h"

#include <array>
#include <ios>
#include <cstdint>
#include <memory>
#include <cstddef>
#include <fstream>
#include <string>
#include <boost/predef.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#if !defined(BOOST_ENDIAN_BIG_BYTE_AVAILABLE) && !defined(BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE)
#error Byte order undefined or unknown. Currently only BOOST_ENDIAN_BIG_BYTE and BOOST_ENDIAN_LITTLE_BYTE are supported.
#endif

inline constexpr size_t STL_FACET_NUMBYTES = 4ul * 3ul * 4ul + 2ul;
// as there is no 'float32_t' standard, we assume the systems 'float'
// is a 'binary32' aka 'single' standard IEEE 32-bit floating point type
union stl_facet {
  static_assert(sizeof(unsigned char) == sizeof(uint8_t), "existence check");
  unsigned char data8[ STL_FACET_NUMBYTES ];
  struct facet_data {
    float i, j, k;
    float x1, y1, z1;
    float x2, y2, z2;
    float x3, y3, z3;
    uint16_t attribute_byte_count;
  } data;
};

static_assert(offsetof(stl_facet::facet_data, attribute_byte_count) == 4ul * 3ul * 4ul,
              "Invalid padding in stl_facet");

#if BOOST_ENDIAN_BIG_BYTE
static void uint32_byte_swap(unsigned char *p) {
# if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3) || defined(__clang__)
  uint32_t& x = *reinterpret_cast<uint32_t *>(p);
  x = __builtin_bswap32(x);
# elif defined(_MSC_VER)
  uint32_t& x = *reinterpret_cast<uint32_t *>(p);
  x = _byteswap_ulong(x);
# else
  std::swap(*p, *(p + 3));
  std::swap(*(p + 1), *(p + 2));
# endif
}

static void uint32_byte_swap(uint32_t& x) {
  uint32_byte_swap(reinterpret_cast<unsigned char *>(&x));
}
#endif // if BOOST_ENDIAN_BIG_BYTE

static void read_stl_facet(std::ifstream& f, stl_facet& facet) {
  f.read((char *)facet.data8, STL_FACET_NUMBYTES);
  if (static_cast<size_t>(f.gcount()) < STL_FACET_NUMBYTES) {
    throw std::ios_base::failure("facet data truncated");
  }
#if BOOST_ENDIAN_BIG_BYTE
  for (int i = 0; i < 12; ++i) {
    uint32_byte_swap(facet.data8 + i * 4);
  }
  // we ignore attribute byte count
#endif
}

std::unique_ptr<PolySet> import_stl(const std::string& filename, const Location& loc) {
  // Open file and position at the end
  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
  if (!f.good()) {
    LOG(message_group::Warning,
        "Can't open import file '%1$s', import() at line %2$d",
        filename, loc.firstLine());
    return PolySet::createEmpty();
  }

  uint32_t facenum = 0;
  boost::regex ex_sfe(R"(^\s*solid|^\s*facet|^\s*endfacet)");
  boost::regex ex_outer("^\\s*outer loop$");
  boost::regex ex_loopend("^\\s*endloop$");
  boost::regex ex_vertex("^\\s*vertex");
  boost::regex ex_vertices(
    R"(^\s*vertex\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)\s*$)");
  boost::regex ex_endsolid("^\\s*endsolid");

  bool binary = false;
  std::streampos file_size = f.tellg();
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
  if(!binary) facenum=0;
  PolySetBuilder builder(0, facenum);
  f.seekg(0);

  char data[5];
  f.read(data, 5);
  if (!binary && !f.eof() && f.good() && !memcmp(data, "solid", 5)) {
    int i = 0;
    int lineno = 1;
    std::array<std::array<double, 3>, 3> vdata;
    std::string line;

    auto AsciiError = [&](const auto& errstr){
        LOG(message_group::Error, loc, "",
            "STL line %1$s, %2$s line '%3$s' importing file '%4$s'",
            lineno, errstr, line, filename);
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
      } else if (boost::regex_search(line, results, ex_vertices) &&
                 results.size() >= 4) {
        try {
          for (int v = 0; v < 3; ++v) {
            vdata.at(i).at(v) = boost::lexical_cast<double>(results[v + 1]);
          }
          if (++i == 3) {
            builder.beginPolygon(3);
            for(int j=0;j<3;j++) {
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
      while (!f.eof() ) {
        stl_facet facet;
        try {
          read_stl_facet(f, facet);
        } catch (const std::ios_base::failure& ex) {
          if (f.eof()) break;
          throw;
        }
        builder.appendPolygon({
                Vector3d(facet.data.x1, facet.data.y1, facet.data.z1),
                Vector3d(facet.data.x2, facet.data.y2, facet.data.z2),
                Vector3d(facet.data.x3, facet.data.y3, facet.data.z3)
        });
      }
    } catch (const std::ios_base::failure& ex) {
      int64_t offset = -1;
      try { offset = f.tellg(); } catch (...) {}
      if (offset < 0) {
        LOG(message_group::Error, loc, "",
            "Binary STL '%1$s' error: %3$s",
            filename, ex.what());
      } else {
        LOG(message_group::Error, loc, "",
            "Binary STL '%1$s' error at byte %2$s: %3$s",
            filename, offset, ex.what());
      }
      return PolySet::createEmpty();
    }
  } else {
    LOG(message_group::Error, loc, "",
        "STL format not recognized in '%1$s'.", filename);
    return PolySet::createEmpty();
  }
  return builder.build();
}

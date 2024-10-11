#include "io/import.h"
#include "Feature.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"
#include "core/AST.h"
#include <system_error>
#include <map>
#include <ios>
#include <cstdint>
#include <memory>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

// References:
// http://www.geomview.org/docs/html/OFF.html

std::unique_ptr<PolySet> import_off(const std::string& filename, const Location& loc)
{
  boost::regex ex_magic(R"(^(ST)?(C)?(N)?(4)?(n)?OFF( BINARY)? *)");
  // XXX: are ST C N always in order?
  boost::regex ex_cr(R"(\r$)");
  boost::regex ex_comment(R"(\s*#.*$)");
  boost::smatch results;

  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);

  int lineno = 0;
  std::string line;

  auto AsciiError = [&](const auto& errstr){
    LOG(message_group::Error, loc, "",
    "OFF File line %1$s, %2$s line '%3$s' importing file '%4$s'",
    lineno, errstr, line, filename);
  };

  auto getline_clean = [&](const auto& errstr){
    do {
      lineno++;
      std::getline(f, line);
      if (line.empty() && f.eof()) {
        AsciiError(errstr);
        return false;
      }
      // strip DOS line endings
      if (boost::regex_search(line, results, ex_cr)) {
        line = line.erase(results.position(), results[0].length());
      }
      // strip comments
      if (boost::regex_search(line, results, ex_comment)) {
        line = line.erase(results.position(), results[0].length());
      }
      boost::trim(line);
    } while (line.empty());

    return true;
  };

  auto getcolor = [&](const auto& word){
    int c;
    if (boost::contains(word, ".")) {
      float f;
#ifdef __cpp_lib_to_chars
      auto result = std::from_chars(word.data(), word.data() + word.length(), f);
      if (result.ec != std::errc{}) {
        AsciiError("Parse error");
        return 0;
      }
#else
      // fall back for pre C++17
      std::istringstream istr(word);
      istr.imbue(std::locale("C"));
      istr >> f;
      if (istr.peek() != EOF) {
        AsciiError("Parse error");
        return 0;
      }
#endif
      c = (int)(f * 255);
    } else {
      c = boost::lexical_cast<int>(word);
    }
    return c;
  };


  if (!f.good()) {
    AsciiError("File error");
    return PolySet::createEmpty();
  }

  bool got_magic = false;
  // defaults
  bool has_normals = false;
  bool has_color = false;
  bool has_textures = false;
  bool has_ndim = false;
  bool is_binary = false;
  unsigned int dimension = 3;

  if (line.empty() && !getline_clean("bad header: end of file")) {
      return PolySet::createEmpty();
  }

  if (boost::regex_search(line, results, ex_magic) > 0) {
    got_magic = true;
    // Remove the matched part, we might have numbers next.
    line = line.erase(0, results[0].length());
    has_normals = results[3].matched;
    has_color = results[2].matched;
    has_textures = results[1].matched;
    is_binary = results[6].matched;
    if (results[4].matched)
      dimension = 4;
    has_ndim = results[5].matched;
  }

  // TODO: handle binary format
  if (is_binary) {
    AsciiError("binary OFF format not supported");
    return PolySet::createEmpty();
  }

  std::vector<std::string> words;

  if (has_ndim) {
    if (line.empty() && !getline_clean("bad header: end of file")) {
        return PolySet::createEmpty();
    }
    boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
    if (f.eof() || words.size() < 1) {
      AsciiError("bad header: missing Ndim");
      return PolySet::createEmpty();
    }
    line = line.erase(0, words[0].length() + ((words.size() > 1) ? 1 : 0));
    try {
      dimension = boost::lexical_cast<unsigned int>(words[0]) + dimension - 3;
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("bad header: bad data for Ndim");
      return PolySet::createEmpty();
    }
  }

  PRINTDB("Header flags: N:%d C:%d ST:%d Ndim:%d B:%d", has_normals % has_color % has_textures % dimension % is_binary);

  if (dimension != 3) {
    AsciiError((boost::format("unhandled vertex dimensions (%d)") % dimension).str().c_str());
    return PolySet::createEmpty();
  }

  if (line.empty() && !getline_clean("bad header: end of file")) {
      return PolySet::createEmpty();
  }

  boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
  if (f.eof() || words.size() < 3) {
    AsciiError("bad header: missing data");
    return PolySet::createEmpty();
  }

  unsigned long vertices_count;
  unsigned long faces_count;
  unsigned long edges_count;
  unsigned long vertex = 0;
  unsigned long face = 0;
  try {
    vertices_count = boost::lexical_cast<unsigned long>(words[0]);
    faces_count = boost::lexical_cast<unsigned long>(words[1]);
    edges_count = boost::lexical_cast<unsigned long>(words[2]);
    (void)edges_count; // ignored
  } catch (const boost::bad_lexical_cast& blc) {
    AsciiError("bad header: bad data");
    return PolySet::createEmpty();
  }

  if (f.eof() || vertices_count < 1 || faces_count < 1) {
    AsciiError("bad header: not enough data");
    return PolySet::createEmpty();
  }

  PRINTDB("%d vertices, %d faces, %d edges.", vertices_count % faces_count % edges_count);

  auto ps = PolySet::createEmpty();
  ps->vertices.reserve(vertices_count);
  ps->indices.reserve(faces_count);

  while ((!f.eof()) && (vertex++ < vertices_count)) {
    if (!getline_clean("reading vertices: end of file")) {
      return PolySet::createEmpty();
    }

    boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
    if (words.size() < 3) {
      AsciiError("can't parse vertex: not enough data");
      return PolySet::createEmpty();
    }

    try {
      Vector3d v = {0, 0, 0};
      int i;
      for (i = 0; i < dimension; i++) {
        v[i]= boost::lexical_cast<double>(words[i]);
      }
      //PRINTDB("Vertex[%ld] = { %f, %f, %f }", vertex % v[0] % v[1] % v[2]);
      if (has_normals) {
        ; // TODO words[i++]
        i += 0;
      }
      if (has_color) {
        ; // TODO: Meshlab appends color there, probably to allow gradients
        i += 3; // 4?
      }
      if (has_textures) {
        ; // TODO words[i++]
      }
      ps->vertices.push_back(v);
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("can't parse vertex: bad data");
      return PolySet::createEmpty();
    }
  }

  auto logged_color_warning = false;

  while (!f.eof() && (face++ < faces_count)) {
    if (!getline_clean("reading faces: end of file")) {
      return PolySet::createEmpty();
    }

    boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
    if (words.size() < 1) {
      AsciiError("can't parse face: not enough data");
      return PolySet::createEmpty();
    }

    std::map<Color4f, int32_t> color_indices;
    try {
      unsigned long face_size=boost::lexical_cast<unsigned long>(words[0]);
      unsigned long i;
      if (words.size() - 1 < face_size) {
        AsciiError("can't parse face: missing indices");
        return PolySet::createEmpty();
      }
      size_t face_idx = ps->indices.size();
      ps->indices.emplace_back().reserve(face_size);
      //PRINTDB("Index[%d] [%d] = { ", face % n);
      for (i = 0; i < face_size; i++) {
        int ind=boost::lexical_cast<int>(words[i+1]);
        //PRINTDB("%d, ", ind);
        if (ind >= 0 && ind < vertices_count) {
          ps->indices.back().push_back(ind);
        } else {
          AsciiError((boost::format("ignored bad face vertex index: %d") % ind).str().c_str());
        }
      }
      //PRINTD("}");
      if (words.size() >= face_size + 4) {
        i = face_size + 1;
        // handle optional color info (r g b [a])
        int r=getcolor(words[i++]);
        int g=getcolor(words[i++]);
        int b=getcolor(words[i++]);
        int a=i < words.size() ? getcolor(words[i++]) : 255;
        Color4f color(r, g, b, a);

        auto iter_pair = color_indices.insert_or_assign(color, ps->colors.size());
        if (iter_pair.second) ps->colors.push_back(color); // inserted
        ps->color_indices.resize(face_idx, -1);
        ps->color_indices.push_back(iter_pair.first->second);
      }
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("can't parse face: bad data");
      return PolySet::createEmpty();
    }
  }
  if (!ps->color_indices.empty()) {
    ps->color_indices.resize(ps->indices.size(), -1);
  }

  //PRINTDB("PS: %ld vertices, %ld indices", ps->vertices.size() % ps->indices.size());
  return ps;
}

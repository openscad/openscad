#include "import.h"
#include "PolySet.h"
#include "printutils.h"
#include "AST.h"
#include <fstream>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

// References:
// http://www.geomview.org/docs/html/OFF.html

std::unique_ptr<PolySet> import_off(const std::string& filename, const Location& loc)
{
  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);

  int lineno = 1;
  std::string line;

  auto AsciiError = [&](const auto& errstr){
    LOG(message_group::Error, loc, "",
    "OFF File line %1$s, %2$s line '%3$s' importing file '%4$s'",
    lineno, errstr, line, filename);
  };

  if (!f.good()) {
    AsciiError("File error");
    return std::make_unique<PolySet>(3);
  }

  boost::regex ex_magic(R"(^(ST)?(C)?(N)?(4)?(n)?OFF( BINARY)? *)");
  // XXX: are ST C N always in order?
  // XXX: should we accept trailing whitespace? comment?
  boost::regex ex_comment(R"(^\s*#)");
  boost::smatch results;

  bool got_magic = false;
  // defaults
  bool has_normals = false;
  bool has_color = false;
  bool has_textures = false;
  bool has_ndim = false;
  bool is_binary = false;
  unsigned int dimension = 3;

  std::getline(f, line);
  if (f.eof()) {
    AsciiError("bad header");
    return std::make_unique<PolySet>(3);
  }
  if (boost::regex_search(line, results, ex_magic) > 0) {
    got_magic = true;
    // Remove the matched part, we might have numbers next.
    line = line.erase(0, results[0].length());
    has_normals = results[3].length() > 0;
    has_color = results[2].length() > 0;
    has_textures = results[1].length() > 0;
    is_binary = results[6].length() > 0;
    if (results[4].length())
      dimension = 4;
    if (results[5].length())
      has_ndim = true;
  }

  // TODO: handle binary format
  if (is_binary) {
    AsciiError("unhandled binary format");
    return std::make_unique<PolySet>(3);
  }

  // TODO: handle comments in the header
  if (line.length() < 1) {
    lineno++;
    std::getline(f, line);
  }
  std::vector<std::string> words;

  if (has_ndim) {
    boost::split(words, line, boost::is_any_of(" \t"));
    if (f.eof() || words.size() < 1) {
      AsciiError("bad header");
      return std::make_unique<PolySet>(3);
    }
    line = line.erase(0, words[0].length() + ((words.size() > 1) ? 1 : 0));
    dimension = boost::lexical_cast<unsigned int>(words[0]) + dimension - 3;
  }

  PRINTDB("Header flags: N:%d C:%d ST:%d Ndim:%d B:%d", has_normals % has_color % has_textures % dimension % is_binary);

  if (dimension != 3) {
    AsciiError("unhandled vertex dimensions");
    return std::make_unique<PolySet>(3);
  }

  // TODO: handle comments in the header
  if (line.length() < 1) {
    lineno++;
    std::getline(f, line);
  }

  boost::split(words, line, boost::is_any_of(" \t"));
  if (f.eof() || words.size() < 3) {
    AsciiError("bad header");
    return std::make_unique<PolySet>(3);
  }

  unsigned long vertices_count;
  unsigned long faces_count;
  unsigned long edges_count;
  try {
    vertices_count = boost::lexical_cast<unsigned long>(words[0]);
    faces_count = boost::lexical_cast<unsigned long>(words[1]);
    edges_count = boost::lexical_cast<unsigned long>(words[2]);
    (void)edges_count; // ignored
  } catch (const boost::bad_lexical_cast& blc) {
    AsciiError("bad header");
    return std::make_unique<PolySet>(3);
  }

  if (f.eof() || vertices_count < 1 || faces_count < 1) {
    AsciiError("bad header");
    return std::make_unique<PolySet>(3);
  }

  PRINTDB("%d vertices, %d faces, %d edges.", vertices_count % faces_count % edges_count);

  auto ps = std::make_unique<PolySet>(3);
  ps->vertices.reserve(vertices_count);
  ps->indices.reserve(faces_count);

  while ((!f.eof()) && (vertices_count--)) {
    lineno++;
    std::getline(f, line);
    if (line.length() == 0 || boost::regex_search(line, ex_comment))
      continue;

    boost::trim(line);
    boost::split(words, line, boost::is_any_of(" \t"));
    if (words.size() < 3) {
      AsciiError("can't parse vertex");
      return std::make_unique<PolySet>(3);
    }

    try {
      Vector3d v;
      for (int i = 0; i < 3; i++) {
        v[i]= boost::lexical_cast<double>(words[i]);
      }
      int o = dimension;
      if (has_normals) {
        ; // TODO words[o++]
        o += 0;
      }
      if (has_color) {
        ; // TODO: Meshlab appends color there, probably to allow gradients
        o += 3; // 4?
      }
      if (has_textures) {
        ; // TODO
      }
      ps->vertices.push_back(v);
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("can't parse vertex");
      return std::make_unique<PolySet>(3);
    }
  }

  while (!f.eof() && faces_count--) {
    lineno++;
    std::getline(f, line);
    if (line.length() == 0 || boost::regex_search(line, ex_comment))
      continue;

    boost::trim(line);
    boost::split(words, line, boost::is_any_of(" \t"));
    if (words.size() < 1) {
      AsciiError("can't parse face");
      return std::make_unique<PolySet>(3);
    }

    try {
      unsigned long n=boost::lexical_cast<unsigned long>(words[0]);
      if (words.size() - 1 < n) {
        AsciiError("can't parse face");
        return std::make_unique<PolySet>(3);
      }
      ps->indices.emplace_back().reserve(n);

      for (int i = 0; i < n; i++) {
        int ind=boost::lexical_cast<int>(words[i+1]);
        if(ind >= 0 && ind < vertices_count)
          ps->indices.back().push_back(ind);
      }
      if (words.size() >= n + 4) {
        // TODO: handle optional color info
        int r=boost::lexical_cast<int>(words[n+1]);
        int g=boost::lexical_cast<int>(words[n+2]);
        int b=boost::lexical_cast<int>(words[n+3]);
      }
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("can't parse vertex");
      return std::make_unique<PolySet>(3);
    }
  }

  f.close();
  return ps;
}

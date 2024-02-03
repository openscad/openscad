#include "import.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "printutils.h"
#include "AST.h"
#include <fstream>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

std::unique_ptr<PolySet> import_off(const std::string& filename, const Location& loc)
{
  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);

  int lineno = 1;
  std::string line;
  //boost::smatch results;

  auto AsciiError = [&](const auto& errstr){
    LOG(message_group::Error, loc, "",
    "OFF File line %1$s, %2$s line '%3$s' importing file '%4$s'",
    lineno, errstr, line, filename);
  };

  if (!f.good()) {
    AsciiError("File error");
    return std::make_unique<PolySet>(3);
  }

  boost::regex ex_magic(R"(^OFF$|^COFF$)"); // TODO: 4OFF?
  boost::regex ex_comment(R"(^\s*#)");



  std::getline(f, line);
  if (f.eof() || !boost::regex_search(line, ex_magic)) {
    AsciiError("bad header");
    return std::make_unique<PolySet>(3);
  }

  std::getline(f, line);
  std::vector<std::string> words;

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
    fprintf(stderr, __FILE__ ": OFF: %d %d %d\n\n", vertices_count, faces_count, edges_count);
  } catch (const boost::bad_lexical_cast& blc) {
    AsciiError("bad header");
    return std::make_unique<PolySet>(3);
  }
  PolySetBuilder builder(vertices_count, faces_count);
  // The builder merges identical vertices, so just keep a table around
  //std::array<Vector3d, vertices_count> vertices;

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
      // TODO: Meshlab appends color there, probably to allow gradients
      fprintf(stderr, __FILE__ ": v: %f %f %f\n\n", v[0], v[1], v[2]);
      builder.vertexIndex(v); // expect to get subsequent numbers starting from zero
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
      fprintf(stderr, __FILE__ ": F: %d\n", n);
      builder.appendPoly(n);

      for (int i = 0; i < n; i++) {
        int ind=boost::lexical_cast<int>(words[i+1]);
        fprintf(stderr, __FILE__ ": f: %d %d\n\n", ind, builder.numVertices());
        if(ind >= 0 && ind < builder.numVertices())
          builder.appendVertex(ind);
      }
      if (words.size() >= n + 4) {
        // optional color info
        int r=boost::lexical_cast<int>(words[n+1]);
        int g=boost::lexical_cast<int>(words[n+2]);
        int b=boost::lexical_cast<int>(words[n+3]);
        // TODO: handle this somehow?
      }
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("can't parse vertex");
      return std::make_unique<PolySet>(3);
    }
  }

  f.close();
  return builder.build();
}

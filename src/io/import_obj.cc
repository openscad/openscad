#include "import.h"
#include "PolySet.h"
#include <fstream>
#include <vector>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

PolySet *import_obj(const std::string& filename, const Location& loc) {
  std::unique_ptr<PolySet> p = std::make_unique<PolySet>(3);

  
  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary );
  if (!f.good()) {
    LOG(message_group::Warning,
        "Can't open import file '%1$s', import() at line %2$d",
        filename, loc.firstLine());
    return p.release();
  }
  std::vector<Vector3d> pts;
  boost::regex ex_comment(R"(^\s*#)");
  boost::regex ex_v( R"(^\s*v\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)\s*$)");
  boost::regex ex_f( R"(^\s*f\s+(.*)$)");
  boost::regex ex_vt( R"(^\s*vt)");
  boost::regex ex_vn( R"(^\s*vn)");
  boost::regex ex_mtllib( R"(^\s*mtllib)");
  boost::regex ex_usemtl( R"(^\s*usemtl)");
  boost::regex ex_o( R"(^\s*o)");
  boost::regex ex_s( R"(^\s*s)");
  boost::regex ex_g( R"(^\s*g)");
  int lineno = 1;
  std::string line;

  auto AsciiError = [&](const auto& errstr){
    LOG(message_group::Error, loc, "",
    "OBJ File line %1$s, %2$s line '%3$s' importing file '%4$s'",
    lineno, errstr, line, filename);
  };

  while (!f.eof()) {
    lineno++;
    std::getline(f, line);
    boost::trim(line);

    boost::smatch results;
    if (line.length() == 0 || boost::regex_search(line, ex_comment)) {
      continue;
    } else if (boost::regex_search(line, results, ex_v) && results.size() >= 4) {
      try {
	Vector3d v;
        for (int i = 0; i < 3; i++) {
          v[i]= boost::lexical_cast<double>(results[i + 1]); 
        }
        pts.push_back(v);
      } catch (const boost::bad_lexical_cast& blc) {
        AsciiError("can't parse vertex");
        return new PolySet(3);
      }
    } else if (boost::regex_search(line, results, ex_f) && results.size() >= 2) {
      p->append_poly();
      std::string args=results[1];
      std::vector<std::string> words;
      boost::split(words, results[1], boost::is_any_of(" \t"));
      for (const std::string& word : words) {
	int ind=boost::lexical_cast<int>(word);
        if(ind >= 1 && ind  <= pts.size())
          p->append_vertex(pts[ind-1][0], pts[ind-1][1], pts[ind-1][2]);
        else
          LOG(message_group::Warning, "Index %1$d out of range in Line %2$d", filename, lineno);
      }

    } else if (boost::regex_search(line, results, ex_vt)) { // ignore texture coords
    } else if (boost::regex_search(line, results, ex_vn)) { // ignore normal coords
    } else if (boost::regex_search(line, results, ex_mtllib)) { // ignore material lib
    } else if (boost::regex_search(line, results, ex_usemtl)) { // ignore usemtl
    } else if (boost::regex_search(line, results, ex_o)) { // ignore object name
    } else if (boost::regex_search(line, results, ex_s)) { // ignore smooting
    } else if (boost::regex_search(line, results, ex_g)) { // ignore group name
    } else {
      LOG(message_group::Warning, "Unrecognized Line  %1$s in line Line %2$d", line, lineno);
    }
  }
  return p.release();
}

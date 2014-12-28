#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iostream>
#include <locale.h>

#include "cgalutils.h"



// Nef polyhedron are using CGAL_Kernel3 (Cartesian<Gmpq>)
// Triangulation will use Epick

static void export_stl(const Polygons &triangles, std::ostream &output)
{
  setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
  output << "solid OpenSCAD_Model\n";
  BOOST_FOREACH(const Polygon &p, triangles) {
    assert(p.size() == 3); // STL only allows triangles
    std::stringstream stream;
    stream << p[0][0] << " " << p[0][1] << " " << p[0][2];
    std::string vs1 = stream.str();
    stream.str("");
    stream << p[1][0] << " " << p[1][1] << " " << p[1][2];
    std::string vs2 = stream.str();
    stream.str("");
    stream << p[2][0] << " " << p[2][1] << " " << p[2][2];
    std::string vs3 = stream.str();
    if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
      // The above condition ensures that there are 3 distinct vertices, but
      // they may be collinear. If they are, the unit normal is meaningless
      // so the default value of "1 0 0" can be used. If the vertices are not
      // collinear then the unit normal must be calculated from the
      // components.
      Vector3d normal = (p[1] - p[0]).cross(p[2] - p[0]);
      normal.normalize();
      output << "  facet normal " << normal[0] << " " << normal[1] << " " << normal[2] << "\n";
      output << "    outer loop\n";
		
      BOOST_FOREACH(const Vector3d &v, p) {
        output << "      vertex " << v[0] << " " << v[1] << " " << v[2] << "\n";
      }
      output << "    endloop\n";
      output << "  endfacet\n";
    }
  }
  output << "endsolid OpenSCAD_Model\n";
  setlocale(LC_NUMERIC, "");      // Set default locale
}


/*!
  file format: 
  1. polygon coordinates (x,y,z) are comma separated (+/- spaces) and 
  each coordinate is on a separate line
  2. each polygon is separated by one or more blank lines
*/
bool import_polygon(PolyholeK &polyhole, const std::string &filename)
{
  std::ifstream ifs(filename.c_str());
  if (!ifs) return false;

  std::string line;
  PolygonK polygon;
  while (std::getline(ifs, line)) {
    std::stringstream ss(line);
    double X = 0.0, Y = 0.0, Z = 0.0;
    if (!(ss >> X)) {
      //ie blank lines => flag start of next polygon 
      if (polygon.size() > 0) polyhole.push_back(polygon);
      polygon.clear();
      continue;
    }
    char c = ss.peek();  
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces before comma
    if (c == ',') {ss.read(&c, 1); c = ss.peek();} //gobble comma
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces after comma
    if (!(ss >> Y)) {
      std::cerr << "Y error\n";
      return false;
    }
    c = ss.peek();
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces before comma
    if (c == ',') {ss.read(&c, 1); c = ss.peek();} //gobble comma
    while (c == ' ') {ss.read(&c, 1); c = ss.peek();} //gobble spaces after comma
    if (!(ss >> Z)) {
      std::cerr << "Z error\n";
      return false;
    }
    polygon.push_back(Vertex3K(X, Y, Z));
  }
  if (polygon.size() > 0) polyhole.push_back(polygon);
  ifs.close();
  return true;
}
//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  PolyholeK polyhole;
  K::Vector_3 *normal = NULL;
  if (argc >= 2) {
    if (!import_polygon(polyhole, argv[1])) {
      std::cerr << "Error importing polygon" << std::endl;
      exit(1);
    }
    std::cerr << "Imported " << polyhole.size() << " polygons" << std::endl;

    if (argc == 3) {
      std::vector<std::string> strs;
      std::vector<double> normalvec;
      std::string arg(argv[2]);
      boost::split(strs, arg, boost::is_any_of(","));
      assert(strs.size() == 3);
      BOOST_FOREACH(const std::string &s, strs) normalvec.push_back(boost::lexical_cast<double>(s));
      normal = new K::Vector_3(normalvec[0], normalvec[1], normalvec[2]);
   }
  }
  else {
    //construct two non-intersecting nested polygons  
    PolygonK polygon1;
    polygon1.push_back(Vertex3K(0,0,0));
    polygon1.push_back(Vertex3K(2,0,0));
    polygon1.push_back(Vertex3K(2,2,0));
    polygon1.push_back(Vertex3K(0,2,0));
    PolygonK polygon2;
    polygon2.push_back(Vertex3K(0.5,0.5,0));
    polygon2.push_back(Vertex3K(1.5,0.5,0));
    polygon2.push_back(Vertex3K(1.5,1.5,0));
    polygon2.push_back(Vertex3K(0.5,1.5,0));
    polyhole.push_back(polygon1);
    polyhole.push_back(polygon2);
  }

  Polygons triangles;
  bool ok = CGALUtils::tessellatePolygonWithHoles(polyhole, triangles, normal);
  std::cerr << "Tessellated into " << triangles.size() << " triangles" << std::endl;

  export_stl(triangles, std::cout);
}

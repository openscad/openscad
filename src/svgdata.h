#ifndef SVGDATA_H_
#define SVGDATA_H_

#include <string>
#include <polyset.h>
#include <dxfdata.h>
#include <libxml++/libxml++.h>

class SVGData{
public:
  SVGData(std::string filename);
  ~SVGData();

  class PolySet* convertToPolyset(int fn);
  void traverse_subtree(const xmlpp::Node* node);

private:
  void start_path();
  void close_path();
  void add_point(double x, double y);

  void parse_path_description(Glib::ustring d);
  std::vector<float> get_params(std::string str);
  void render_line_to(float x0, float y0, float x1, float y1);
  void render_curve_to(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);

  std::string filename;
  xmlpp::DomParser* parser;
  DxfData *dxfdata;
  PolySet *p;
  Grid2d<int>* grid;
  int first_point, last_point;
};

#endif

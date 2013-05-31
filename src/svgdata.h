#ifndef SVGDATA_H_
#define SVGDATA_H_

#include <string>
#include <polyset.h>
#include <dxfdata.h>

class SVGData{
public:
  SVGData(std::string filename);
  ~SVGData();

  class PolySet* convertToPolyset(int fn);

private:
  void start_path();
  void close_path();
  void add_point(double x, double y);

  std::string filename;
  DxfData *dxfdata;
  PolySet *p;
  Grid2d<int>* grid;
  int first_point, last_point;
};

#endif

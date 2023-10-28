#pragma once

#include "node.h"
#include "Value.h"

class LinearExtrudeNode : public AbstractPolyNode
{
public:
  VISITABLE();
  LinearExtrudeNode(double height);
  LinearExtrudeNode(double height, bool center);
  LinearExtrudeNode(double height, double twist);
  LinearExtrudeNode(double height, int convexity, double twist);
  LinearExtrudeNode(double height, int convexity, double twist, double scale);
  LinearExtrudeNode(double height, int convexity, double twist, std::vector<double>& scale);
  LinearExtrudeNode(double height, bool center, int convexity, double twist);
  LinearExtrudeNode(double height, bool center, int convexity, double twist, double scale);
  LinearExtrudeNode(double height, bool center, int convexity, double twist, std::vector<double>& scale);
  LinearExtrudeNode() : LinearExtrudeNode(new ModuleInstantiation("linear_extrude")) {}
  LinearExtrudeNode(ModuleInstantiation *mi) : AbstractPolyNode(mi) {
  }
  std::string toString() const override;
  std::string name() const override { return "linear_extrude"; }
  void set_twist(double value);
  void set_slices(unsigned int value);
  void set_segments(unsigned int value);

  double height = 100.0;
  double origin_x = 0.0, origin_y = 0.0;
  double fn = 0.0, fs = 0.0, fa = 0.0;
  double scale_x = 1.0, scale_y = 1.0;
  double twist = 0.0;
  unsigned int convexity = 1u;
  unsigned int slices = 1u, segments = 0u;
  bool has_twist = false, has_slices = false, has_segments = false;
  bool center = false;

  Filename filename;
  std::string layername;
};

#pragma once
#include "node.h" // AbstractNode
#include "primitives.h" // point2d, point3d
#include "TextNode.h" // FreetypeRenderer::Params
// #include "linalg.h"
#include "SourceFile.h"
#include "Camera.h"

// public:

//   void setViewOptions();

//   ViewOptions viewOptions;
  // RenderSettings render_settings;
//   RenderSettings renderSettings;
//   Camera camera;
extern bool parse(SourceFile *& file, const std::string& text, const std::string& filename, const std::string& mainFile, int debug);

class Primitive2D;
class Primitive3D;

class OpenSCADContext: public std::enable_shared_from_this<OpenSCADContext>
{
public:
  auto get_shared_ptr() { return shared_from_this(); } 
  OpenSCADContext();
  OpenSCADContext(std::string filename); 
  static std::shared_ptr<OpenSCADContext> context(std::string filename) 
  {
    return std::make_shared<OpenSCADContext>(filename);
  }
  static std::shared_ptr<OpenSCADContext> from_scad_file(std::string filename);
  int export_file(std::string output_file);
  // std::shared_ptr<OpenSCADContext> export_file(std::string output_file);
  // std::shared_ptr<OpenSCADContext> export_file(std::shared_ptr<AbstractNode> root_node, std::string output_file);
  std::shared_ptr<OpenSCADContext> use_file(std::string filename);
  std::shared_ptr<OpenSCADContext> append_scad(std::string scad_txt, std::string name = "untitled.scad");
  std::shared_ptr<OpenSCADContext> append(std::shared_ptr<Primitive2D> primitive);
  std::shared_ptr<OpenSCADContext> append(std::shared_ptr<Primitive3D> primitive);
  std::shared_ptr<OpenSCADContext> set_fn(double fn);
  std::shared_ptr<OpenSCADContext> set_fa(double fa);
  std::shared_ptr<OpenSCADContext> set_fs(double fs);

  std::shared_ptr<Primitive2D> square();
  std::shared_ptr<Primitive2D> square(double size);
  std::shared_ptr<Primitive2D> square(double size, bool center);
  std::shared_ptr<Primitive2D> square(double x, double y);
  std::shared_ptr<Primitive2D> square(double x, double y, bool center);

  std::shared_ptr<Primitive2D> circle();
  std::shared_ptr<Primitive2D> circle(double size);

  std::shared_ptr<Primitive2D> polygon(std::vector<point2d>& points);
  std::shared_ptr<Primitive2D> polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths);
  std::shared_ptr<Primitive2D> polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity);

  // std::shared_ptr<Primitive2D> text();
  std::shared_ptr<Primitive2D> text(FreetypeRenderer::Params& params); 
  std::shared_ptr<Primitive2D> text(std::string& text);
  std::shared_ptr<Primitive2D> text(std::string& text, int size);
  std::shared_ptr<Primitive2D> text(std::string& text, int size, std::string& font);

  std::shared_ptr<Primitive3D> cube();
  std::shared_ptr<Primitive3D> cube(double size);
  std::shared_ptr<Primitive3D> cube(double size, bool center);
  std::shared_ptr<Primitive3D> cube(double x, double y, double z);
  std::shared_ptr<Primitive3D> cube(double x, double y, double z, bool center);

  std::shared_ptr<Primitive3D> sphere();
  std::shared_ptr<Primitive3D> sphere(double r);

  std::shared_ptr<Primitive3D> cylinder();
  std::shared_ptr<Primitive3D> cylinder(double r, double h);
  std::shared_ptr<Primitive3D> cylinder(double r, double h, bool center);
  std::shared_ptr<Primitive3D> cylinder(double r1, double r2, double h);
  std::shared_ptr<Primitive3D> cylinder(double r1, double r2, double h, bool center);

  std::shared_ptr<Primitive3D> polyhedron();
  std::shared_ptr<Primitive3D> polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces);
  std::shared_ptr<Primitive3D> polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity);
  
  std::shared_ptr<AbstractNode> root_node = nullptr;
  std::shared_ptr<SourceFile> source_file;
  std::shared_ptr<Camera> camera;

  double fn = 0;
  double fa = 12;
  double fs = 2;
};

class OSObject 
{
public:
  std::shared_ptr<OpenSCADContext> context;
  std::shared_ptr<AbstractNode> node;
  std::shared_ptr<AbstractNode> transformations = nullptr;
};

class Operator: public OSObject, public std::enable_shared_from_this<Operator>
{
  //can we check if the invocation result will be an operator by looking at loaded module definition?
  std::shared_ptr<Operator> from_scad(std::string scad_txt);
};


class Primitive2D: public OSObject, public std::enable_shared_from_this<Primitive2D>
{
public:
  auto get_shared_ptr() { return shared_from_this(); } 
  static std::shared_ptr<Primitive2D> square(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive2D> square(std::shared_ptr<OpenSCADContext> ctx, double size);
  static std::shared_ptr<Primitive2D> square(std::shared_ptr<OpenSCADContext> ctx, double size, bool center);
  static std::shared_ptr<Primitive2D> square(std::shared_ptr<OpenSCADContext> ctx, double x, double y);
  static std::shared_ptr<Primitive2D> square(std::shared_ptr<OpenSCADContext> ctx, double x, double y, bool center);

  static std::shared_ptr<Primitive2D> circle(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive2D> circle(std::shared_ptr<OpenSCADContext> ctx, double size);

  static std::shared_ptr<Primitive2D> polygon(std::shared_ptr<OpenSCADContext> ctx, std::vector<point2d>& points);
  static std::shared_ptr<Primitive2D> polygon(std::shared_ptr<OpenSCADContext> ctx, std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths);
  static std::shared_ptr<Primitive2D> polygon(std::shared_ptr<OpenSCADContext> ctx, std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity);

  // static std::shared_ptr<Primitive2D> text(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive2D> text(std::shared_ptr<OpenSCADContext> ctx, FreetypeRenderer::Params& params); 
  static std::shared_ptr<Primitive2D> text(std::shared_ptr<OpenSCADContext> ctx, std::string& text);
  static std::shared_ptr<Primitive2D> text(std::shared_ptr<OpenSCADContext> ctx, std::string& text, int size);
  static std::shared_ptr<Primitive2D> text(std::shared_ptr<OpenSCADContext> ctx, std::string& text, int size, std::string& font);

  static std::shared_ptr<Primitive2D> from_scad(std::string& text);
  std::shared_ptr<Primitive2D> op(std::shared_ptr<Operator> op);

  std::shared_ptr<Primitive2D> set_debug(std::string modifier);

  std::shared_ptr<Primitive2D> color(std::string color);
  std::shared_ptr<Primitive2D> color(std::string colorname, float alpha);
  std::shared_ptr<Primitive2D> color(int red, int green, int blue);
  std::shared_ptr<Primitive2D> color(int red, int green, int blue, int alpha);
  std::shared_ptr<Primitive2D> color(float red, float green, float blue);
  std::shared_ptr<Primitive2D> color(float red, float green, float blue, float alpha);
  std::shared_ptr<Primitive2D> scale(double num);
  std::shared_ptr<Primitive2D> scale(std::vector<double>& vec);
  std::shared_ptr<Primitive2D> translate(std::vector<double>& vec);
  std::shared_ptr<Primitive2D> rotate(double angle);
  std::shared_ptr<Primitive2D> rotate(double angle, std::vector<double>& vec);
  std::shared_ptr<Primitive2D> rotate(std::vector<double>& angle);
  std::shared_ptr<Primitive2D> mirror(std::vector<double>& vec);
  std::shared_ptr<Primitive2D> multmatrix(std::vector<std::vector<double>>& vec);

  std::shared_ptr<Primitive2D> union_(Primitive2D* primitive);
  std::shared_ptr<Primitive2D> intersection(Primitive2D* primitive);
  std::shared_ptr<Primitive2D> difference(Primitive2D* primitive);

  std::shared_ptr<Primitive2D> minkowski();
  std::shared_ptr<Primitive2D> minkowski(int convexity);
  std::shared_ptr<Primitive2D> hull();
  std::shared_ptr<Primitive2D> fill();

  std::shared_ptr<Primitive2D> resize(std::vector<double>& newsize);
  std::shared_ptr<Primitive2D> resize(std::vector<double>& newsize, bool autosize);
  std::shared_ptr<Primitive2D> resize(std::vector<double>& newsize, bool autosize, int convexity);
  std::shared_ptr<Primitive2D> resize(std::vector<double>& newsize, std::vector<bool>& autosize);
  std::shared_ptr<Primitive2D> resize(std::vector<double>& newsize, std::vector<bool>& autosize, int convexity); 

  // static shared_ptr<TextMetrics> textmetrics();
  // static shared_ptr<FontMetrics> fontmetrics();

  std::shared_ptr<Primitive3D> linear_extrude(double height);
  std::shared_ptr<Primitive3D> linear_extrude(double height, bool center);
  std::shared_ptr<Primitive3D> linear_extrude(double height, double twist);
  std::shared_ptr<Primitive3D> linear_extrude(double height, int convexity, double twist);
  std::shared_ptr<Primitive3D> linear_extrude(double height, int convexity, double twist, double scale);
  std::shared_ptr<Primitive3D> linear_extrude(double height, int convexity, double twist, std::vector<double>& scale);
  std::shared_ptr<Primitive3D> linear_extrude(double height, bool center, int convexity, double twist);
  std::shared_ptr<Primitive3D> linear_extrude(double height, bool center, int convexity, double twist, double scale);
  std::shared_ptr<Primitive3D> linear_extrude(double height, bool center, int convexity, double twist, std::vector<double>& scale);
  std::shared_ptr<Primitive3D> rotate_extrude(double angle);
  std::shared_ptr<Primitive3D> rotate_extrude(double angle, int convexity);
  std::shared_ptr<Primitive2D> offset(std::string op, double delta);
  std::shared_ptr<Primitive2D> offset(std::string op, double delta, bool chamfer);

  // std::shared_ptr<Primitive2D> invoke_scad_module(std::string code);
};

class Primitive3D: public OSObject, public std::enable_shared_from_this<Primitive3D>
{
public:
  auto get_shared_ptr() { return shared_from_this();}

  static std::shared_ptr<Primitive3D> cube(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive3D> cube(std::shared_ptr<OpenSCADContext> ctx, double size);
  static std::shared_ptr<Primitive3D> cube(std::shared_ptr<OpenSCADContext> ctx, double size, bool center);
  static std::shared_ptr<Primitive3D> cube(std::shared_ptr<OpenSCADContext> ctx, double x, double y, double z);
  static std::shared_ptr<Primitive3D> cube(std::shared_ptr<OpenSCADContext> ctx, double x, double y, double z, bool center);

  static std::shared_ptr<Primitive3D> sphere(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive3D> sphere(std::shared_ptr<OpenSCADContext> ctx, double r);

  static std::shared_ptr<Primitive3D> cylinder(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive3D> cylinder(std::shared_ptr<OpenSCADContext> ctx, double r, double h);
  static std::shared_ptr<Primitive3D> cylinder(std::shared_ptr<OpenSCADContext> ctx, double r, double h, bool center);
  static std::shared_ptr<Primitive3D> cylinder(std::shared_ptr<OpenSCADContext> ctx, double r1, double r2, double h);
  static std::shared_ptr<Primitive3D> cylinder(std::shared_ptr<OpenSCADContext> ctx, double r1, double r2, double h, bool center);

  static std::shared_ptr<Primitive3D> polyhedron(std::shared_ptr<OpenSCADContext> ctx);
  static std::shared_ptr<Primitive3D> polyhedron(std::shared_ptr<OpenSCADContext> ctx, std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces);
  static std::shared_ptr<Primitive3D> polyhedron(std::shared_ptr<OpenSCADContext> ctx, std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity);
  // static std::shared_ptr<Primitive3D> from_scad(std::string& text);

  // std::shared_ptr<Primitive2D> op(std::shared_ptr<Operator> operator);
  std::shared_ptr<Primitive3D> set_debug(std::string modifier);

  std::shared_ptr<Primitive3D> color(std::string color);
  std::shared_ptr<Primitive3D> color(std::string colorname, float alpha);
  std::shared_ptr<Primitive3D> color(int red, int green, int blue);
  std::shared_ptr<Primitive3D> color(int red, int green, int blue, int alpha);
  std::shared_ptr<Primitive3D> color(float red, float green, float blue);
  std::shared_ptr<Primitive3D> color(float red, float green, float blue, float alpha);
  std::shared_ptr<Primitive3D> scale(double num);
  std::shared_ptr<Primitive3D> scale(std::vector<double>& vec);
  std::shared_ptr<Primitive3D> translate(std::vector<double>& vec);
  std::shared_ptr<Primitive3D> rotate(double angle);
  std::shared_ptr<Primitive3D> rotate(double angle, std::vector<double>& vec);
  std::shared_ptr<Primitive3D> rotate(std::vector<double>& angle);
  std::shared_ptr<Primitive3D> mirror(std::vector<double>& vec);
  std::shared_ptr<Primitive3D> multmatrix(std::vector<std::vector<double>>& vec);
  
  std::shared_ptr<Primitive3D> union_(Primitive3D *primitive);
  std::shared_ptr<Primitive3D> intersection(Primitive3D *primitive);
  std::shared_ptr<Primitive3D> difference(Primitive3D *primitive);

  std::shared_ptr<Primitive3D> minkowski();
  std::shared_ptr<Primitive3D> minkowski(int convexity);
  std::shared_ptr<Primitive3D> hull();
  std::shared_ptr<Primitive3D> fill();

  std::shared_ptr<Primitive3D> resize(std::vector<double>& newsize);
  std::shared_ptr<Primitive3D> resize(std::vector<double>& newsize, bool autosize);
  std::shared_ptr<Primitive3D> resize(std::vector<double>& newsize, bool autosize, int convexity);
  std::shared_ptr<Primitive3D> resize(std::vector<double>& newsize, std::vector<bool>& autosize);
  std::shared_ptr<Primitive3D> resize(std::vector<double>& newsize, std::vector<bool>& autosize, int convexity); 
  std::shared_ptr<Primitive3D> roof();
  std::shared_ptr<Primitive2D> projection();
  std::shared_ptr<Primitive2D> projection(bool cut);
};
  // std::shared_ptr<Primitive2D> invoke_scad_module(std::string code);

// class OpenSCAD2DTransform
// {

// }

// class OpenSCAD3DTransform
// {

// }

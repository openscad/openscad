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
  std::shared_ptr<OpenSCADContext> export_file(std::string output_file);
  // std::shared_ptr<OpenSCADContext> export_file(std::shared_ptr<AbstractNode> root_node, std::string output_file);
  std::shared_ptr<OpenSCADContext> use_file(std::string filename);
  std::shared_ptr<OpenSCADContext> append_scad(std::string scad_txt, std::string name = "untitled.scad");
  std::shared_ptr<OpenSCADContext> append(std::shared_ptr<Primitive2D> primitive);
  std::shared_ptr<OpenSCADContext> append(std::shared_ptr<Primitive3D> primitive);

  std::shared_ptr<AbstractNode> root_node = nullptr;
  std::shared_ptr<SourceFile> source_file;
  std::shared_ptr<Camera> camera;
};

class OSObject 
{
public:
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
  static std::shared_ptr<Primitive2D> square();
  static std::shared_ptr<Primitive2D> square(double size);
  static std::shared_ptr<Primitive2D> square(double size, bool center);
  static std::shared_ptr<Primitive2D> square(double x, double y);
  static std::shared_ptr<Primitive2D> square(double x, double y, bool center);

  static std::shared_ptr<Primitive2D> circle();
  static std::shared_ptr<Primitive2D> circle(double size);

  static std::shared_ptr<Primitive2D> polygon(std::vector<point2d>& points);
  static std::shared_ptr<Primitive2D> polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths);
  static std::shared_ptr<Primitive2D> polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity);

  // static std::shared_ptr<Primitive2D> text();
  static std::shared_ptr<Primitive2D> text(FreetypeRenderer::Params& params); 
  static std::shared_ptr<Primitive2D> text(std::string& text);
  static std::shared_ptr<Primitive2D> text(std::string& text, int size);
  static std::shared_ptr<Primitive2D> text(std::string& text, int size, std::string& font);

  // static std::shared_ptr<Primitive2D> from_scad(std::string& text);
  // std::shared_ptr<Primitive2D> op(std::shared_ptr<Operator> operator);

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

  static std::shared_ptr<Primitive3D> cube();
  static std::shared_ptr<Primitive3D> cube(double size);
  static std::shared_ptr<Primitive3D> cube(double size, bool center);
  static std::shared_ptr<Primitive3D> cube(double x, double y, double z);
  static std::shared_ptr<Primitive3D> cube(double x, double y, double z, bool center);

  static std::shared_ptr<Primitive3D> sphere();
  static std::shared_ptr<Primitive3D> sphere(double r);

  static std::shared_ptr<Primitive3D> cylinder();
  static std::shared_ptr<Primitive3D> cylinder(double r, double h);
  static std::shared_ptr<Primitive3D> cylinder(double r, double h, bool center);
  static std::shared_ptr<Primitive3D> cylinder(double r1, double r2, double h);
  static std::shared_ptr<Primitive3D> cylinder(double r1, double r2, double h, bool center);

  static shared_ptr<Primitive3D> polyhedron();
  static shared_ptr<Primitive3D> polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces);
  static shared_ptr<Primitive3D> polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity);
  // static std::shared_ptr<Primitive3D> from_scad(std::string& text);

  // std::shared_ptr<Primitive2D> op(std::shared_ptr<Operator> operator);
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

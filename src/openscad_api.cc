#include "openscad_api.h"
#include "primitives.h"
#include "TransformNode.h"
#include "CsgOpNode.h"
#include "CgalAdvNode.h"
#include "SurfaceNode.h"
#include "TextNode.h"
#include "RenderNode.h"
#include "RoofNode.h"
#include "LinearExtrudeNode.h"
#include "RotateExtrudeNode.h"
#include "ColorNode.h"
#include "OffsetNode.h"
#include "SourceFile.h"

OpenSCADContext::OpenSCADContext(std::string filename)
{
  fs::path filepath;
  try {
    filepath = fs::absolute(fs::path(filename));
    auto path = filepath.parent_path().string();
    auto name = filepath.filename().string();
    source_file = std::make_shared<SourceFile>(path, name);
  } catch (...) {
    LOG(message_group::Error, "Error: file access denied");
    // return false;
  }
}

std::shared_ptr<OpenSCADContext> OpenSCADContext::use_file(std::string filename)
{
  this->source_file->registerUse(filename, Location::NONE);
  return this->shared_from_this();
}

template<typename T, typename S>
std::shared_ptr<T> init_(std::shared_ptr<S> shape)
{
  auto primitive = std::make_shared<T>();
  primitive->node = shape;
  primitive->transformations = shape;
  return primitive;
}
template<typename T, typename S>
std::shared_ptr<T> update_(T* primitive, std::shared_ptr<S> node)
{
  node->children.push_back(primitive->transformations);
  primitive->transformations = node;
  return primitive->get_shared_ptr();
}
template<typename T>
std::shared_ptr<T> color(T *primitive, Color4f *color)
{
  auto node = std::make_shared<ColorNode>(color);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> color(T *primitive, std::string colorname)
{
  auto node = std::make_shared<ColorNode>(colorname);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> color(T *primitive, std::string colorname, float alpha)
{
  auto node = std::make_shared<ColorNode>(colorname, alpha);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> color(T *primitive, int red, int green, int blue)
{
  auto node = std::make_shared<ColorNode>(red, green, blue);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> color(T *primitive, int red, int green, int blue, int alpha)
{
  auto node = std::make_shared<ColorNode>(red, green, blue, alpha);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> color(T *primitive, float red, float green, float blue)
{
  auto node = std::make_shared<ColorNode>(red, green, blue);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> color(T *primitive, float red, float green, float blue, float alpha)
{
  auto node = std::make_shared<ColorNode>(red, green, blue, alpha);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> scale(T *primitive, double num)
{
  auto node = TransformNode::scale(num);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> scale(T *primitive, std::vector<double>& vec)
{
  auto node = TransformNode::scale(vec);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> translate(T *primitive, std::vector<double>& vec)
{
  auto node = TransformNode::translate(vec);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> rotate(T *primitive, double angle)
{
  auto node = TransformNode::rotate(angle);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> rotate(T *primitive, double angle, std::vector<double>& vec)
{
  auto node = TransformNode::rotate(angle, vec);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> rotate(T *primitive, std::vector<double>& angle)
{
  auto node = TransformNode::rotate(angle);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> mirror(T *primitive, std::vector<double>& vec)
{
  auto node = TransformNode::mirror(vec);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> multmatrix(T *primitive, std::vector<std::vector<double>>& vec)
{
  auto node = TransformNode::multmatrix(vec);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> union_(T* primitive, T* primitive2)
{
  auto node = CsgOpNode::union_();
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> intersection(T* primitive, T* primitive2)
{
  auto node = CsgOpNode::intersection();
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> difference(T* primitive, T* primitive2)
{
  auto node = CsgOpNode::difference();
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> minkowski(T* primitive)
{
  auto node = CgalAdvNode::minkowski();
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> minkowski(T* primitive, int convexity)
{
  auto node = CgalAdvNode::minkowski(convexity);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> hull(T* primitive)
{
  auto node = CgalAdvNode::hull();
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> fill(T* primitive)
{
  auto node = CgalAdvNode::fill();
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> resize(T* primitive, std::vector<double>& newsize)
{
  auto node = CgalAdvNode::resize(newsize);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> resize(T* primitive, std::vector<double>& newsize, bool autosize)
{
  auto node = CgalAdvNode::resize(newsize, autosize);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> resize(T* primitive, std::vector<double>& newsize, bool autosize, int convexity)
{
  auto node = CgalAdvNode::resize(newsize, autosize, convexity);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> resize(T* primitive, std::vector<double>& newsize, std::vector<bool>& autosize)
{
  auto node = CgalAdvNode::resize(newsize, autosize);
  return update_<T>(primitive, node);
}
template<typename T>
std::shared_ptr<T> resize(T* primitive, std::vector<double>& newsize, std::vector<bool>& autosize, int convexity)
{
  auto node = CgalAdvNode::resize(newsize, autosize, convexity);
  return update_<T>(primitive, node);
}

std::shared_ptr<Primitive3D> Primitive3D::cube()
{
  auto shape = std::make_shared<CubeNode>();
  return ::init_<Primitive3D, CubeNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(double size)
{
  auto shape = std::make_shared<CubeNode>(size);
  return ::init_<Primitive3D, CubeNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(double size, bool center)
{
  auto shape = std::make_shared<CubeNode>(size, center);
  return ::init_<Primitive3D, CubeNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(double x, double y, double z)
{
  auto shape = std::make_shared<CubeNode>(x, y, z);
  return ::init_<Primitive3D, CubeNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(double x, double y, double z, bool center)
{
  auto shape = std::make_shared<CubeNode>(x, y, z, center);
  return ::init_<Primitive3D, CubeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive3D::sphere()
{
  auto shape = std::make_shared<SphereNode>();
  return ::init_<Primitive3D, SphereNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::sphere(double r)
{
  auto shape = std::make_shared<SphereNode>(r);
  return ::init_<Primitive3D, SphereNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive3D::cylinder()
{
  auto shape = std::make_shared<CylinderNode>();
  return ::init_<Primitive3D, CylinderNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(double r, double h)
{
  auto shape = std::make_shared<CylinderNode>(r, h);
  return ::init_<Primitive3D, CylinderNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(double r, double h, bool center)
{
  auto shape = std::make_shared<CylinderNode>(r, h, center);
  return ::init_<Primitive3D, CylinderNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(double r1, double r2, double h)
{
  auto shape = std::make_shared<CylinderNode>(r1, r2, h);
  return ::init_<Primitive3D, CylinderNode>(shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(double r1, double r2, double h, bool center)
{
  auto shape = std::make_shared<CylinderNode>(r1, r2, h, center);
  return ::init_<Primitive3D, CylinderNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive3D::polyhedron()
{
  auto shape = std::make_shared<PolyhedronNode>();
  return ::init_<Primitive3D, PolyhedronNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive3D::polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces)
{
  auto shape = std::make_shared<PolyhedronNode>(points, faces);
  return ::init_<Primitive3D, PolyhedronNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive3D::polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity)
{
  auto shape = std::make_shared<PolyhedronNode>(points, faces, convexity);
  return ::init_<Primitive3D, PolyhedronNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square()
{
  auto shape = std::make_shared<SquareNode>();
  return ::init_<Primitive2D, SquareNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(double size)
{
  auto shape = std::make_shared<SquareNode>(size);
  return ::init_<Primitive2D, SquareNode>(shape);
}
std::shared_ptr<Primitive2D> Primitive2D::square(double size, bool center)
{
  auto shape = std::make_shared<SquareNode>(size, center);
  return ::init_<Primitive2D, SquareNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(double x, double y)
{
  auto shape = std::make_shared<SquareNode>(x, y);
  return ::init_<Primitive2D, SquareNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(double x, double y, bool center)
{
  auto shape = std::make_shared<SquareNode>(x, y, center);
  return ::init_<Primitive2D, SquareNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::circle()
{
  auto shape = std::make_shared<CircleNode>();
  return ::init_<Primitive2D, CircleNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::circle(double size)
{
  auto shape = std::make_shared<CircleNode>(size);
  return ::init_<Primitive2D, CircleNode>(shape);
}

// std::shared_ptr<Primitive2D> Primitive2D::polygon(){
//   auto primitive = std::make_shared<Primitive2D>();
//   primitive->node = Polygon::polygon();
//   return primitive;
// }
std::shared_ptr<Primitive2D> Primitive2D::polygon(std::vector<point2d>& points)
{
  auto shape = std::make_shared<PolygonNode>(points);
  return ::init_<Primitive2D, PolygonNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths)
{
  auto shape = std::make_shared<PolygonNode>(points, paths);
  return ::init_<Primitive2D, PolygonNode>(shape);
}
std::shared_ptr<Primitive2D> Primitive2D::polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity)
{
  auto shape = std::make_shared<PolygonNode>(points, paths, convexity);
  return ::init_<Primitive2D, PolygonNode>(shape);
}
// std::shared_ptr<Primitive2D> Primitive2D::text(){
//   auto primitive = std::make_shared<Primitive2D>();
//   primitive->node = std::make_shared<TextNode>();
//   return primitive;
// }
std::shared_ptr<Primitive2D> Primitive2D::text(FreetypeRenderer::Params& params)
{
  auto shape = std::make_shared<TextNode>(params);
  return ::init_<Primitive2D, TextNode>(shape);
} 

std::shared_ptr<Primitive2D> Primitive2D::text(std::string& text)
{
  auto shape = std::make_shared<TextNode>(text);
  return ::init_<Primitive2D, TextNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::text(std::string& text, int size)
{
  auto shape = std::make_shared<TextNode>(text, size);
  return ::init_<Primitive2D, TextNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::text(std::string& text, int size, std::string& font)
{
  auto shape = std::make_shared<TextNode>(text, size, font);
  return ::init_<Primitive2D, TextNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, double twist)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, twist);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, int convexity, double twist)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, convexity, twist);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, int convexity, double twist, double scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, int convexity, double twist, std::vector<double>& scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center, int convexity, double twist)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center, convexity, twist);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center, int convexity, double twist, double scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center, int convexity, double twist, std::vector<double>& scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::rotate_extrude(double angle)
{
  auto shape = std::make_shared<RotateExtrudeNode>(angle);
  node->children.push_back(this->transformations);
  // this->transformations = node;
  return ::init_<Primitive3D, RotateExtrudeNode>(shape);
}

std::shared_ptr<Primitive3D> Primitive2D::rotate_extrude(double angle, int convexity)
{
  auto shape = std::make_shared<RotateExtrudeNode>(angle, convexity);
  node->children.push_back(this->transformations);
  // this->transformations = node;
  return ::init_<Primitive3D, RotateExtrudeNode>(shape);
}

std::shared_ptr<Primitive2D> Primitive2D::offset(std::string op, double delta)
{
  auto node = std::make_shared<OffsetNode>(op, delta);
  return update_<Primitive2D>(this, node);
}

std::shared_ptr<Primitive2D> Primitive2D::offset(std::string op, double delta, bool chamfer)
{
  auto node = std::make_shared<OffsetNode>(op, delta, chamfer);
  return update_<Primitive2D>(this, node);
}

std::shared_ptr<Primitive2D> Primitive2D::color(Color4f *color) 
{ 
  return ::color<Primitive2D>(this, color); 
}
std::shared_ptr<Primitive2D> Primitive2D::color(std::string color)
{ 
  return ::color<Primitive2D>(this, color); 
}
std::shared_ptr<Primitive2D> Primitive2D::color(std::string colorname, float alpha)
{ 
  return ::color<Primitive2D>(this, colorname, alpha);
}
std::shared_ptr<Primitive2D> Primitive2D::color(int red, int green, int blue)
{ 
  return ::color<Primitive2D>(this, red, green, blue);
}
std::shared_ptr<Primitive2D> Primitive2D::color(int red, int green, int blue, int alpha)
{ 
  return ::color<Primitive2D>(this, red, green, blue, alpha);
}
std::shared_ptr<Primitive2D> Primitive2D::color(float red, float green, float blue)
{ 
  return ::color<Primitive2D>(this, red, green, blue);
}
std::shared_ptr<Primitive2D> Primitive2D::color(float red, float green, float blue, float alpha)
{ 
  return ::color<Primitive2D>(this, red, green, blue, alpha);
}
std::shared_ptr<Primitive2D> Primitive2D::scale(double num)
{ 
  return ::scale<Primitive2D>(this, num);
}
std::shared_ptr<Primitive2D> Primitive2D::scale(std::vector<double>& vec)
{ 
  return ::scale<Primitive2D>(this, vec);
}
std::shared_ptr<Primitive2D> Primitive2D::translate(std::vector<double>& vec)
{ 
  return ::translate<Primitive2D>(this, vec);
}
std::shared_ptr<Primitive2D> Primitive2D::rotate(double angle)
{ 
  return ::rotate<Primitive2D>(this, angle);
}
std::shared_ptr<Primitive2D> Primitive2D::rotate(double angle, std::vector<double>& vec)
{ 
  return ::rotate<Primitive2D>(this, angle, vec);
}
std::shared_ptr<Primitive2D> Primitive2D::rotate(std::vector<double>& angle)
{ 
  return ::rotate<Primitive2D>(this, angle);
}
std::shared_ptr<Primitive2D> Primitive2D::mirror(std::vector<double>& vec)
{ 
  return ::mirror<Primitive2D>(this, vec);
}
std::shared_ptr<Primitive2D> Primitive2D::multmatrix(std::vector<std::vector<double>>& vec)
{ 
  return ::multmatrix<Primitive2D>(this, vec);
}

std::shared_ptr<Primitive2D> Primitive2D::union_(Primitive2D* primitive)
{
  return ::union_<Primitive2D>(this, primitive);
}
std::shared_ptr<Primitive2D> Primitive2D::intersection(Primitive2D* primitive)
{
  return ::intersection<Primitive2D>(this, primitive);
}
std::shared_ptr<Primitive2D> Primitive2D::difference(Primitive2D* primitive)
{
  return ::difference<Primitive2D>(this, primitive);
}

std::shared_ptr<Primitive2D> Primitive2D::minkowski()
{
  return ::minkowski<Primitive2D>(this);
}
std::shared_ptr<Primitive2D> Primitive2D::minkowski(int convexity)
{
  return ::minkowski<Primitive2D>(this, convexity);
}
std::shared_ptr<Primitive2D> Primitive2D::hull()
{
  return ::hull<Primitive2D>(this);
}
std::shared_ptr<Primitive2D> Primitive2D::fill()
{
  return ::fill<Primitive2D>(this);
}

std::shared_ptr<Primitive2D> Primitive2D::resize(std::vector<double>& newsize)
{
  return ::resize<Primitive2D>(this, newsize);
}
std::shared_ptr<Primitive2D> Primitive2D::resize(std::vector<double>& newsize, bool autosize)
{
  return ::resize<Primitive2D>(this, newsize, autosize);
}
std::shared_ptr<Primitive2D> Primitive2D::resize(std::vector<double>& newsize, bool autosize, int convexity)
{
  return ::resize<Primitive2D>(this, newsize, autosize, convexity);
}
std::shared_ptr<Primitive2D> Primitive2D::resize(std::vector<double>& newsize, std::vector<bool>& autosize)
{
  return ::resize<Primitive2D>(this, newsize, autosize);
}
std::shared_ptr<Primitive2D> Primitive2D::resize(std::vector<double>& newsize, std::vector<bool>& autosize, int convexity)
{
  return ::resize<Primitive2D>(this, newsize, autosize, convexity);
}

std::shared_ptr<Primitive3D> Primitive3D::color(Color4f *color) 
{ 
  return ::color<Primitive3D>(this, color); 
}
std::shared_ptr<Primitive3D> Primitive3D::color(std::string color)
{ 
  return ::color<Primitive3D>(this, color); 
}
std::shared_ptr<Primitive3D> Primitive3D::color(std::string colorname, float alpha)
{ 
  return ::color<Primitive3D>(this, colorname, alpha);
}
std::shared_ptr<Primitive3D> Primitive3D::color(int red, int green, int blue)
{ 
  return ::color<Primitive3D>(this, red, green, blue);
}
std::shared_ptr<Primitive3D> Primitive3D::color(int red, int green, int blue, int alpha)
{ 
  return ::color<Primitive3D>(this, red, green, blue, alpha);
}
std::shared_ptr<Primitive3D> Primitive3D::color(float red, float green, float blue)
{ 
  return ::color<Primitive3D>(this, red, green, blue);
}
std::shared_ptr<Primitive3D> Primitive3D::color(float red, float green, float blue, float alpha)
{ 
  return ::color<Primitive3D>(this, red, green, blue, alpha);
}
std::shared_ptr<Primitive3D> Primitive3D::scale(double num)
{ 
  return ::scale<Primitive3D>(this, num);
}
std::shared_ptr<Primitive3D> Primitive3D::scale(std::vector<double>& vec)
{ 
  return ::scale<Primitive3D>(this, vec);
}
std::shared_ptr<Primitive3D> Primitive3D::translate(std::vector<double>& vec)
{ 
  return ::translate<Primitive3D>(this, vec);
}
std::shared_ptr<Primitive3D> Primitive3D::rotate(double angle)
{ 
  return ::rotate<Primitive3D>(this, angle);
}
std::shared_ptr<Primitive3D> Primitive3D::rotate(double angle, std::vector<double>& vec)
{ 
  return ::rotate<Primitive3D>(this, angle, vec);
}
std::shared_ptr<Primitive3D> Primitive3D::rotate(std::vector<double>& angle)
{ 
  return ::rotate<Primitive3D>(this, angle);
}
std::shared_ptr<Primitive3D> Primitive3D::mirror(std::vector<double>& vec)
{ 
  return ::mirror<Primitive3D>(this, vec);
}
std::shared_ptr<Primitive3D> Primitive3D::multmatrix(std::vector<std::vector<double>>& vec)
{ 
  return ::multmatrix<Primitive3D>(this, vec);
}

std::shared_ptr<Primitive3D> Primitive3D::union_(Primitive3D* primitive)
{
  return ::union_<Primitive3D>(this, primitive);
}
std::shared_ptr<Primitive3D> Primitive3D::intersection(Primitive3D* primitive)
{
  return ::intersection<Primitive3D>(this, primitive);
}
std::shared_ptr<Primitive3D> Primitive3D::difference(Primitive3D* primitive)
{
  return ::difference<Primitive3D>(this, primitive);
}

std::shared_ptr<Primitive3D> Primitive3D::minkowski()
{
  return ::minkowski<Primitive3D>(this);
}
std::shared_ptr<Primitive3D> Primitive3D::minkowski(int convexity)
{
  return ::minkowski<Primitive3D>(this, convexity);
}
std::shared_ptr<Primitive3D> Primitive3D::hull()
{
  return ::hull<Primitive3D>(this);
}
std::shared_ptr<Primitive3D> Primitive3D::fill()
{
  return ::fill<Primitive3D>(this);
}

std::shared_ptr<Primitive3D> Primitive3D::resize(std::vector<double>& newsize)
{
  return ::resize<Primitive3D>(this, newsize);
}
std::shared_ptr<Primitive3D> Primitive3D::resize(std::vector<double>& newsize, bool autosize)
{
  return ::resize<Primitive3D>(this, newsize, autosize);
}
std::shared_ptr<Primitive3D> Primitive3D::resize(std::vector<double>& newsize, bool autosize, int convexity)
{
  return ::resize<Primitive3D>(this, newsize, autosize, convexity);
}
std::shared_ptr<Primitive3D> Primitive3D::resize(std::vector<double>& newsize, std::vector<bool>& autosize)
{
  return ::resize<Primitive3D>(this, newsize, autosize);
}
std::shared_ptr<Primitive3D> Primitive3D::resize(std::vector<double>& newsize, std::vector<bool>& autosize, int convexity)
{
  return ::resize<Primitive3D>(this, newsize, autosize, convexity);
}

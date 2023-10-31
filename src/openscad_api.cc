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
#include "handle_dep.h"
#include "BuiltinContext.h"
#include "openscad.h"

#include <fstream>

std::shared_ptr<AbstractNode> eval_source_file(SourceFile* root_file)
{
  auto filename = root_file->getFilename();
  auto fpath = fs::absolute(fs::path(filename));
  auto fparent = fpath.parent_path();
  EvaluationSession session{fparent.string()};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
// #ifdef DEBUG
  // PRINTDB("BuiltinContext:\n%s", builtin_context->dump());
// #endif
  LOG("BuiltinContext:\n%s", builtin_context->dump());

  AbstractNode::resetIndexCounter();
  std::shared_ptr<const FileContext> file_context;
  std::shared_ptr<AbstractNode> absolute_root_node;
  absolute_root_node = root_file->instantiate(*builtin_context, &file_context);
  return absolute_root_node;
}

void merge_root_nodes(std::shared_ptr<AbstractNode> first, std::shared_ptr<AbstractNode> second)
{
  if (first == nullptr || first->children.empty()) 
  {
    first = second;
  } else if (!second->children.empty())
  {
    first->children.insert(first->children.end(), second->children.begin(), second->children.end());
  }
}

OpenSCADContext::OpenSCADContext()
{
  camera = std::make_shared<Camera>();
  root_node = std::make_shared<RootNode>();
}

OpenSCADContext::OpenSCADContext(std::string filename) : OpenSCADContext()
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

std::shared_ptr<OpenSCADContext> OpenSCADContext::from_scad_file(std::string filename)
{
  std::string text;
  std::ifstream ifs(filename);
  if (!ifs.is_open()) {
    LOG("Can't open input file '%1$s'!\n", filename);
    return nullptr;
  }
  handle_dep(filename);
  text = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()); 
  text += "\n\x03\n"; // + commandline_commands;

  SourceFile *root_file = nullptr;
  if (!parse(root_file, text, filename, filename, false)) {
    delete root_file; // parse failed
    root_file = nullptr;
  }
  if (!root_file) {
    LOG("Can't parse file '%1$s'!\n", filename);
    return nullptr;
  } 

  std::shared_ptr<SourceFile> shared_src_ptr(root_file);
  auto context = std::make_shared<OpenSCADContext>();
  context->source_file = shared_src_ptr;

  // add parameter to AST
  // CommentParser::collectParameters(text.c_str(), root_file);
  // if (!cmd.parameterFile.empty() && !cmd.setName.empty()) {
  //   ParameterObjects parameters = ParameterObjects::fromSourceFile(root_file);
  //   ParameterSets sets;
  //   sets.readFile(cmd.parameterFile);
  //   for (const auto& set : sets) {
  //     if (set.name() == cmd.setName) {
  //       parameters.importValues(set);
  //       parameters.apply(root_file);
  //       break;
  //     }
  //   }
  // }

  root_file->handleDependencies();
  context->root_node = eval_source_file(root_file);
  return context;
}

int OpenSCADContext::export_file(std::string filename)
{
  auto root_node = ::find_root(this->root_node);
  return ::export_file(root_node, this->source_file, filename);
}

#define F_MINIMUM 0.01

std::shared_ptr<OpenSCADContext> OpenSCADContext::set_fn(double fn) {
  this->fn = fn;
  return shared_from_this();
}

std::shared_ptr<OpenSCADContext> OpenSCADContext::set_fa(double fa) {
  if (fa < F_MINIMUM) {
    LOG(message_group::Warning, "$fa too small - clamping to %1$f", F_MINIMUM);
    fa = F_MINIMUM;
  }
  this->fa = fa;
  return this->shared_from_this();
}

std::shared_ptr<OpenSCADContext> OpenSCADContext::set_fs(double fs) {
  if (fs < F_MINIMUM) {
    LOG(message_group::Warning, "$fs too small - clamping to %1$f", F_MINIMUM);
    fs = F_MINIMUM;
  }
  this->fs = fs;
  return this->shared_from_this();
}

// std::shared_ptr<OpenSCADContext> OpenSCADContext::export_file(std::string output_file)
// {
//   return this->shared_from_this();
// }

// std::shared_ptr<OpenSCADContext> OpenSCADContext::export_file(std::shared_ptr<AbstractNode> root_node, std::string output_file)
// {
//   return this->shared_from_this();
// }

std::shared_ptr<OpenSCADContext> OpenSCADContext::use_file(std::string filename)
{
  this->source_file->registerUse(filename, Location::NONE);
  return this->shared_from_this();
}

std::shared_ptr<OpenSCADContext> OpenSCADContext::append_scad(std::string text, std::string filename)
{
  // inject fn, fa, fs
  text += "\n\x03\n";// + commandline_commands;

  SourceFile *root_file = nullptr;
  if (!parse(root_file, text, filename, filename, false)) {
    delete root_file; // parse failed
    root_file = nullptr;
  }
  if (!root_file) {
    LOG("Can't parse file '%1$s'!\n", filename);
    return nullptr;
  }
  root_file->handleDependencies();
  auto scad_root_node = eval_source_file(root_file);
  merge_root_nodes(this->root_node, scad_root_node);
  return this->shared_from_this();
}

void set_fragments(std::shared_ptr<OpenSCADContext> context, double& fn, double& fa, double& fs)
{
  if(fn == 0 && context->fn > 0)
  {
    fn = context->fn;
  }
  if(fs == 0 && context->fs > 0)
  {
    fs = context->fs;
  }
  if(fa == 0 && context->fa > 0)
  {
    fa = context->fa;
  }
}

std::shared_ptr<OpenSCADContext> OpenSCADContext::append(std::shared_ptr<Primitive2D> primitive)
{
  // auto name = primitive->node->name();
  // if (name == "circle")
  // {
  //   std::shared_ptr<CircleNode> node =
  //     std::dynamic_pointer_cast<CircleNode> (primitive->node);
  //   set_fragments(this, node->fn, node->fa, node->fs);
  // }
  this->root_node->children.emplace_back(primitive->transformations);
  return this->shared_from_this();
}

std::shared_ptr<OpenSCADContext> OpenSCADContext::append(std::shared_ptr<Primitive3D> primitive)
{
  // auto name = primitive->node->name();
  // if (name == "sphere")
  // {
  //   std::shared_ptr<SphereNode> node =
  //     std::dynamic_pointer_cast<SphereNode> (primitive->node);
  //   set_fragments(this, node->fn, node->fa, node->fs);
  // } else if (name == "cylinder")
  // {
  //   std::shared_ptr<CylinderNode> node =
  //     std::dynamic_pointer_cast<CylinderNode> (primitive->node);
  //   set_fragments(this, node->fn, node->fa, node->fs);
  // }

  this->root_node->children.emplace_back(primitive->transformations);
  return this->shared_from_this();
}

std::shared_ptr<Primitive2D> OpenSCADContext::square()
{
  return Primitive2D::square(shared_from_this());
}
std::shared_ptr<Primitive2D> OpenSCADContext::square(double size)
{
  return Primitive2D::square(shared_from_this(), size);
}
std::shared_ptr<Primitive2D> OpenSCADContext::square(double size, bool center)
{
  return Primitive2D::square(shared_from_this(), size, center);
}
std::shared_ptr<Primitive2D> OpenSCADContext::square(double x, double y)
{
  return Primitive2D::square(shared_from_this(), x, y);
}
std::shared_ptr<Primitive2D> OpenSCADContext::square(double x, double y, bool center)
{
  return Primitive2D::square(shared_from_this(), x, y, center);
}

std::shared_ptr<Primitive2D> OpenSCADContext::circle()
{
  return Primitive2D::circle(shared_from_this());
}
std::shared_ptr<Primitive2D> OpenSCADContext::circle(double size)
{
  return Primitive2D::circle(shared_from_this(), size);
}

std::shared_ptr<Primitive2D> OpenSCADContext::polygon(std::vector<point2d>& points)
{
  return Primitive2D::polygon(shared_from_this(), points);
}
std::shared_ptr<Primitive2D> OpenSCADContext::polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths)
{
  return Primitive2D::polygon(shared_from_this(), points, paths);
}
std::shared_ptr<Primitive2D> OpenSCADContext::polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity)
{
  return Primitive2D::polygon(shared_from_this(), points, paths, convexity);
}

std::shared_ptr<Primitive2D> OpenSCADContext::text(FreetypeRenderer::Params& params)
{
  return Primitive2D::text(shared_from_this(), params);
} 
std::shared_ptr<Primitive2D> OpenSCADContext::text(std::string& text)
{
  return Primitive2D::text(shared_from_this(), text);
}
std::shared_ptr<Primitive2D> OpenSCADContext::text(std::string& text, int size)
{
  return Primitive2D::text(shared_from_this(), text, size);
}
std::shared_ptr<Primitive2D> OpenSCADContext::text(std::string& text, int size, std::string& font)
{
  return Primitive2D::text(shared_from_this(), text, size, font);
}

std::shared_ptr<Primitive3D> OpenSCADContext::cube()
{
  return Primitive3D::cube(shared_from_this());
}
std::shared_ptr<Primitive3D> OpenSCADContext::cube(double size)
{
  return Primitive3D::cube(shared_from_this(), size);
}
std::shared_ptr<Primitive3D> OpenSCADContext::cube(double size, bool center)
{
  return Primitive3D::cube(shared_from_this(), size, center);
}
std::shared_ptr<Primitive3D> OpenSCADContext::cube(double x, double y, double z)
{
  return Primitive3D::cube(shared_from_this(), x, y, z);
}
std::shared_ptr<Primitive3D> OpenSCADContext::cube(double x, double y, double z, bool center)
{
  return Primitive3D::cube(shared_from_this(), x, y, z, center);
}

std::shared_ptr<Primitive3D> OpenSCADContext::sphere()
{
  return Primitive3D::sphere(shared_from_this());
}
std::shared_ptr<Primitive3D> OpenSCADContext::sphere(double r)
{
  return Primitive3D::sphere(shared_from_this(), r);
}

std::shared_ptr<Primitive3D> OpenSCADContext::cylinder()
{
  return Primitive3D::cylinder(shared_from_this());
}
std::shared_ptr<Primitive3D> OpenSCADContext::cylinder(double r, double h)
{
  return Primitive3D::cylinder(shared_from_this(), r, h);
}
std::shared_ptr<Primitive3D> OpenSCADContext::cylinder(double r, double h, bool center)
{
  return Primitive3D::cylinder(shared_from_this(), r, h, center);
}
std::shared_ptr<Primitive3D> OpenSCADContext::cylinder(double r1, double r2, double h)
{
  return Primitive3D::cylinder(shared_from_this(), r1, r2, h);
}
std::shared_ptr<Primitive3D> OpenSCADContext::cylinder(double r1, double r2, double h, bool center)
{
  return Primitive3D::cylinder(shared_from_this(), r1, r2, h, center);
}

std::shared_ptr<Primitive3D> OpenSCADContext::polyhedron()
{
  return Primitive3D::polyhedron(shared_from_this()); 
}
std::shared_ptr<Primitive3D> OpenSCADContext::polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces)
{
  return Primitive3D::polyhedron(shared_from_this(), points, faces); 
}
std::shared_ptr<Primitive3D> OpenSCADContext::polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity)
{
  return Primitive3D::polyhedron(shared_from_this(), points, faces, convexity); 
}

std::shared_ptr<Primitive3D> Primitive3D::set_debug(std::string modifier)
{
  this->node->set_debug(modifier);
  return shared_from_this();
}

std::shared_ptr<Primitive2D> Primitive2D::set_debug(std::string modifier)
{
  this->node->set_debug(modifier);
  return shared_from_this();
}

template<typename T, typename S>
std::shared_ptr<T> init_(std::shared_ptr<OpenSCADContext> context, std::shared_ptr<S> shape)
{
  auto primitive = std::make_shared<T>();
  primitive->node = shape;
  primitive->transformations = shape;
  primitive->context = context;
  return primitive;
}
template<typename T, typename S>
std::shared_ptr<T> update_(T* primitive, std::shared_ptr<S> node)
{
  node->children.push_back(primitive->transformations);
  primitive->transformations = node;
  return primitive->get_shared_ptr();
}
// template<typename T>
// std::shared_ptr<T> color(T *primitive, Color4f *color)
// {
//   auto node = std::make_shared<ColorNode>(color);
//   return update_<T>(primitive, node);
// }
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
  auto ptr = update_<T>(primitive, node);
  node->children.push_back(primitive2->transformations);
  return ptr;
}
template<typename T>
std::shared_ptr<T> intersection(T* primitive, T* primitive2)
{
  auto node = CsgOpNode::intersection();
  auto ptr = update_<T>(primitive, node);
 node->children.push_back(primitive2->transformations);
  return ptr;
}
template<typename T>
std::shared_ptr<T> difference(T* primitive, T* primitive2)
{
  auto node = CsgOpNode::difference();
  auto ptr = update_<T>(primitive, node);
  node->children.push_back(primitive2->transformations);
  return ptr;
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

std::shared_ptr<Primitive3D> Primitive3D::cube(std::shared_ptr<OpenSCADContext> context)
{
  auto shape = std::make_shared<CubeNode>();
  return ::init_<Primitive3D, CubeNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(std::shared_ptr<OpenSCADContext> context, double size)
{
  auto shape = std::make_shared<CubeNode>(size);
  return ::init_<Primitive3D, CubeNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(std::shared_ptr<OpenSCADContext> context, double size, bool center)
{
  auto shape = std::make_shared<CubeNode>(size, center);
  return ::init_<Primitive3D, CubeNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(std::shared_ptr<OpenSCADContext> context, double x, double y, double z)
{
  auto shape = std::make_shared<CubeNode>(x, y, z);
  return ::init_<Primitive3D, CubeNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cube(std::shared_ptr<OpenSCADContext> context, double x, double y, double z, bool center)
{
  auto shape = std::make_shared<CubeNode>(x, y, z, center);
  return ::init_<Primitive3D, CubeNode>(context, shape);
}

std::shared_ptr<Primitive3D> Primitive3D::sphere(std::shared_ptr<OpenSCADContext> context)
{
  auto shape = std::make_shared<SphereNode>();
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, SphereNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::sphere(std::shared_ptr<OpenSCADContext> context, double r)
{
  auto shape = std::make_shared<SphereNode>(r);
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, SphereNode>(context, shape);
}

std::shared_ptr<Primitive3D> Primitive3D::cylinder(std::shared_ptr<OpenSCADContext> context)
{
  auto shape = std::make_shared<CylinderNode>();
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, CylinderNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(std::shared_ptr<OpenSCADContext> context, double r, double h)
{
  auto shape = std::make_shared<CylinderNode>(r, h);
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, CylinderNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(std::shared_ptr<OpenSCADContext> context, double r, double h, bool center)
{
  auto shape = std::make_shared<CylinderNode>(r, h, center);
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, CylinderNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(std::shared_ptr<OpenSCADContext> context, double r1, double r2, double h)
{
  auto shape = std::make_shared<CylinderNode>(r1, r2, h);
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, CylinderNode>(context, shape);
}
std::shared_ptr<Primitive3D> Primitive3D::cylinder(std::shared_ptr<OpenSCADContext> context, double r1, double r2, double h, bool center)
{
  auto shape = std::make_shared<CylinderNode>(r1, r2, h, center);
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive3D, CylinderNode>(context, shape);
}

std::shared_ptr<Primitive3D> Primitive3D::polyhedron(std::shared_ptr<OpenSCADContext> context)
{
  auto shape = std::make_shared<PolyhedronNode>();
  return ::init_<Primitive3D, PolyhedronNode>(context, shape);
}

std::shared_ptr<Primitive3D> Primitive3D::polyhedron(std::shared_ptr<OpenSCADContext> context, std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces)
{
  auto shape = std::make_shared<PolyhedronNode>(points, faces);
  return ::init_<Primitive3D, PolyhedronNode>(context, shape);
}

std::shared_ptr<Primitive3D> Primitive3D::polyhedron(std::shared_ptr<OpenSCADContext> context, std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity)
{
  auto shape = std::make_shared<PolyhedronNode>(points, faces, convexity);
  return ::init_<Primitive3D, PolyhedronNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(std::shared_ptr<OpenSCADContext> context)
{
  auto shape = std::make_shared<SquareNode>();
  return ::init_<Primitive2D, SquareNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(std::shared_ptr<OpenSCADContext> context, double size)
{
  auto shape = std::make_shared<SquareNode>(size);
  return ::init_<Primitive2D, SquareNode>(context, shape);
}
std::shared_ptr<Primitive2D> Primitive2D::square(std::shared_ptr<OpenSCADContext> context, double size, bool center)
{
  auto shape = std::make_shared<SquareNode>(size, center);
  return ::init_<Primitive2D, SquareNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(std::shared_ptr<OpenSCADContext> context, double x, double y)
{
  auto shape = std::make_shared<SquareNode>(x, y);
  return ::init_<Primitive2D, SquareNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::square(std::shared_ptr<OpenSCADContext> context, double x, double y, bool center)
{
  auto shape = std::make_shared<SquareNode>(x, y, center);
  return ::init_<Primitive2D, SquareNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::circle(std::shared_ptr<OpenSCADContext> context)
{
  auto shape = std::make_shared<CircleNode>();
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive2D, CircleNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::circle(std::shared_ptr<OpenSCADContext> context, double size)
{
  auto shape = std::make_shared<CircleNode>(size);
  set_fragments(context, shape->fn, shape->fa, shape->fs);
  return ::init_<Primitive2D, CircleNode>(context, shape);
}

// std::shared_ptr<Primitive2D> Primitive2D::polygon(std::shared_ptr<OpenSCADContext> context, ){
//   auto primitive = std::make_shared<Primitive2D>();
//   primitive->node = Polygon::polygon();
//   return primitive;
// }
std::shared_ptr<Primitive2D> Primitive2D::polygon(std::shared_ptr<OpenSCADContext> context, std::vector<point2d>& points)
{
  auto shape = std::make_shared<PolygonNode>(points);
  return ::init_<Primitive2D, PolygonNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::polygon(std::shared_ptr<OpenSCADContext> context, std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths)
{
  auto shape = std::make_shared<PolygonNode>(points, paths);
  return ::init_<Primitive2D, PolygonNode>(context, shape);
}
std::shared_ptr<Primitive2D> Primitive2D::polygon(std::shared_ptr<OpenSCADContext> context, std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity)
{
  auto shape = std::make_shared<PolygonNode>(points, paths, convexity);
  return ::init_<Primitive2D, PolygonNode>(context, shape);
}
// std::shared_ptr<Primitive2D> Primitive2D::text(){
//   auto primitive = std::make_shared<Primitive2D>();
//   primitive->node = std::make_shared<TextNode>();
//   return primitive;
// }
std::shared_ptr<Primitive2D> Primitive2D::text(std::shared_ptr<OpenSCADContext> context, FreetypeRenderer::Params& params)
{
  auto shape = std::make_shared<TextNode>(params);
  return ::init_<Primitive2D, TextNode>(context, shape);
} 

std::shared_ptr<Primitive2D> Primitive2D::text(std::shared_ptr<OpenSCADContext> context, std::string& text)
{
  auto shape = std::make_shared<TextNode>(text);
  return ::init_<Primitive2D, TextNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::text(std::shared_ptr<OpenSCADContext> context, std::string& text, int size)
{
  auto shape = std::make_shared<TextNode>(text, size);
  return ::init_<Primitive2D, TextNode>(context, shape);
}

std::shared_ptr<Primitive2D> Primitive2D::text(std::shared_ptr<OpenSCADContext> context, std::string& text, int size, std::string& font)
{
  auto shape = std::make_shared<TextNode>(text, size, font);
  return ::init_<Primitive2D, TextNode>(context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, double twist)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, twist);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, int convexity, double twist)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, convexity, twist);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, int convexity, double twist, double scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, int convexity, double twist, std::vector<double>& scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center, int convexity, double twist)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center, convexity, twist);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center, int convexity, double twist, double scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::linear_extrude(double height, bool center, int convexity, double twist, std::vector<double>& scale)
{
  auto shape = std::make_shared<LinearExtrudeNode>(height, center, convexity, twist, scale);
  shape->children.push_back(this->transformations);
  // this->transformations = shape;
  return ::init_<Primitive3D, LinearExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::rotate_extrude(double angle)
{
  auto shape = std::make_shared<RotateExtrudeNode>(angle);
  node->children.push_back(this->transformations);
  // this->transformations = node;
  return ::init_<Primitive3D, RotateExtrudeNode>(this->context, shape);
}

std::shared_ptr<Primitive3D> Primitive2D::rotate_extrude(double angle, int convexity)
{
  auto shape = std::make_shared<RotateExtrudeNode>(angle, convexity);
  node->children.push_back(this->transformations);
  // this->transformations = node;
  return ::init_<Primitive3D, RotateExtrudeNode>(this->context, shape);
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

// std::shared_ptr<Primitive2D> Primitive2D::color(Color4f *color) 
// { 
//   return ::color<Primitive2D>(this, color); 
// }
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

// std::shared_ptr<Primitive3D> Primitive3D::color(Color4f *color) 
// { 
//   return ::color<Primitive3D>(this, color); 
// }
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

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/operators.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/bind_map.h>
#include <nanobind/stl/bind_vector.h>
// #include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "version.h"
#include "ColorMap.h"
#include "openscad.h"
#include "PlatformUtils.h"
#include "LibraryInfo.h"
#include "Feature.h"
#include "SourceFile.h"
#include "LocalScope.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "Assignment.h"
#include "module.h"
#include "node.h"
#include "function.h"
#include "AST.h"
#include "Geometry.h"
#include "primitives.h"
#include "TextNode.h"
#include "TransformNode.h"
#include "CsgOpNode.h"
#include "CgalAdvNode.h"
#include "ProjectionNode.h"
#include "OffsetNode.h"
#include "ColorNode.h"
#include "RenderNode.h"
#include "RoofNode.h"
#include "SurfaceNode.h"
#include "LinearExtrudeNode.h"
#include "RotateExtrudeNode.h"
#include "Camera.h"
#include "Tree.h"
#include "export.h"
#include "Annotation.h"
#include "Expression.h"
#include "openscad_api.h"

namespace nb = nanobind;
using namespace nb::literals;

// NB_MAKE_OPAQUE(std::vector<shared_ptr<Assignment>>);
// NB_MAKE_OPAQUE(std::vector<shared_ptr<AbstractNode>>);


NB_MODULE(openscadpy, m) {
  nb::bind_map<std::map<std::string, bool>>(m, "DictFlags");
  nb::bind_map<std::unordered_map<std::string, std::string>>(m, "DictString");
  nb::bind_map<std::unordered_map<std::string, shared_ptr<UserModule>>>(m, "DictUserModule");
  nb::bind_map<std::unordered_map<std::string, shared_ptr<UserFunction>>>(m, "DictUserFunction");
  nb::bind_map<std::unordered_map<std::string, Annotation *>>(m, "AnnotationMap");
  nb::bind_vector<std::vector<Annotation>>(m, "AssignmentList");
  nb::bind_vector<std::vector<std::string>>(m, "VectorString");
  nb::bind_vector<std::vector<double>>(m, "VectorDouble");
  nb::bind_vector<std::vector<std::vector<double>>>(m, "MatrixDouble");
  nb::bind_vector<std::vector<shared_ptr<Assignment>>>(m, "VectorAssignment");
  nb::bind_vector<std::vector<shared_ptr<AbstractNode>>>(m, "VectorAbstractNode");
  nb::bind_vector<std::vector<shared_ptr<ModuleInstantiation>>>(m, "VectorModuleInstantiation");


  m.def("library_info", &LibraryInfo::info, "Print OpenSCAD library info.");
  m.def("version", &detailed_version_number, "Returns Detailed OpenSCAD version.");
  m.def("display_version", &display_version_number, "Returns Display OpenSCAD version.");
  // m.def("set_color_scheme", &set_color_scheme, "a"_a, "Set Render Color Scheme.");
  // m.def("show_grid", &show_grid, "a" _a, "Show Render Grid.");
  // m.def("get_color_schemes", &get_color_schemes, "Returns Color Schemes.")
  m.def("set_debug", &set_debug, "flag"_a, "Set Debug.");
  m.def("set_trace_depth", &set_trace_depth, "value"_a, "Set trace depth.");
  m.def("set_quiet", &set_quiet, "flag"_a, "Set quiet mode flag.");
  m.def("set_hardwarnings", &set_hardwarnings, "flag"_a, "Set hard warnings flag.");
  m.def("set_render_color_scheme", &set_render_color_scheme, "color_scheme"_a, "Set render color scheme.");
  m.def("set_csglimit", &set_csglimit, "limit"_a, "Set CSG limit.");
  m.def("set_feature", &Feature::enable_feature, "feature"_a, "status"_a, "Enable Experimental Feature.");
  m.def("set_all_features", &Feature::enable_all, "status"_a, "Enable All Experimental Feature - all");

  m.def("get_debug", &get_debug, "Get Debug.");
  m.def("get_trace_depth", &get_trace_depth, "Get trace depth.");
  m.def("get_quiet", &get_quiet, "Get quiet mode flag.");
  m.def("get_hardwarnings", &get_hardwarnings, "Get hard warnings flag.");
  m.def("get_csglimit", &get_csglimit, "Get CSG limit.");
  m.def("get_render_color_scheme", &get_render_color_scheme, "Get render color scheme.");
  m.def("get_features", &Feature::features, "Get Experimental Features.");
  // m.def("get_enabled_features")


  m.def("init_globals", &init_globals, "Initalize OpenSCAD Global.");
  m.def("get_application_path", &PlatformUtils::applicationPath, "Returns OpenSCAD application path.");
  m.def("get_documents_path", &PlatformUtils::documentsPath, "Returns OpenSCAD application path.");
  m.def("get_user_documents_path", &PlatformUtils::userDocumentsPath, "Returns OpenSCAD application path.");
  m.def("get_resource_base_path", &PlatformUtils::resourceBasePath, "Returns OpenSCAD application path.");
  m.def("get_user_library_path", &PlatformUtils::userLibraryPath, "Returns OpenSCAD application path.");
  m.def("get_user_config_path", &PlatformUtils::userConfigPath, "Returns OpenSCAD application path.");
  m.def("get_color_scheme_names", &ColorMap::getColorSchemeNames, "guiOnly"_a = true, "Return color schemes.");

  m.def("get_web_colors", &ColorNode::getWebColors, "Returns SVG Colors.");

  m.def("parse_scad", &parse_scad, "text"_a, "filename"_a = "untitled.scad", "Parse a scad text.");
  m.def("eval_scad", &eval_scad, "root_file"_a, "Evaluate a scad text.");
  m.def("find_root", &find_root, "node"_a, "Find root from tree.");
  m.def("export", &export_file, "root_node"_a, "output_file"_a, "Export png.");
  m.def("context", &OpenSCADContext::context, "filename"_a, "Create OpenSCADContext.");
  m.def("context_from_scad_file", &OpenSCADContext::from_scad_file, "filename"_a, "Create OpenSCADContext from a file.");

  nb::enum_<OpenSCADOperator>(m, "Operator")
    .value("UNION", OpenSCADOperator::UNION)
    .value("INTERSECTION", OpenSCADOperator::INTERSECTION)
    .value("DIFFERENCE", OpenSCADOperator::DIFFERENCE)
    .value("MINKOWSKI", OpenSCADOperator::MINKOWSKI)
    .value("HULL", OpenSCADOperator::HULL)
    .value("FILL", OpenSCADOperator::FILL)
    .value("RESIZE", OpenSCADOperator::RESIZE)
    .export_values();

  nb::class_<OpenSCADContext>(m, "Context")
    // .def(nb::init<std::string>())
    .def("use_file", &OpenSCADContext::use_file, "filename"_a, "Use file.")
    .def("export_file", &OpenSCADContext::export_file, "output_file"_a, "Export file.")
    .def("append_scad", &OpenSCADContext::append_scad, "scad_text"_a, "name"_a = "untitled.scad", "append scad file.")
    .def("append", nb::overload_cast<std::shared_ptr<Primitive2D>>(&OpenSCADContext::append), "primitive"_a, "append 2d Primitive.")
    .def("append", nb::overload_cast<std::shared_ptr<Primitive3D>>(&OpenSCADContext::append),  "primitive"_a, "append 3d Primitive.")
    .def_ro("root_node", &OpenSCADContext::root_node)
    .def_ro("source_file", &OpenSCADContext::source_file); //OpenSCAD


  nb::class_<ExportInfo>(m, "ExportInfo");

  nb::class_<ViewOptions>(m, "ViewOptions")
    .def("names", &ViewOptions::names)
    .def_rw("previewer", &ViewOptions::previewer)
    .def_rw("renderer", &ViewOptions::renderer)
    .def_rw("flags", &ViewOptions::flags);


  nb::class_<Tree>(m, "Tree")
    .def(nb::init<std::shared_ptr<AbstractNode>>())
    .def(nb::init<std::shared_ptr<AbstractNode>, std::string>())
    .def("root", &Tree::root)
    .def("get_document_path", &Tree::getDocumentPath)
    .def("get_string", &Tree::getString, "node"_a, "indent"_a);

  nb::class_<Camera>(m, "Camera")
    .def(nb::init<>())
    .def("__repr__", &Camera::statusText, "Camera status string.")
    .def("setup", &Camera::setup)
    .def("set_projection", nb::overload_cast<std::string&>(&Camera::setProjection))
    .def("reset_view", &Camera::resetView)
    .def("get_vpt", &Camera::getVpt)
    .def("set_vpt", &Camera::setVpt, "x"_a, "y"_a, "z"_a)
    .def("get_vpr", &Camera::getVpr)
    .def("set_vpr", &Camera::setVpr, "x"_a, "y"_a, "z"_a)
    .def("set_vpd", &Camera::setVpd, "d"_a)
    .def("set_vpf", &Camera::setVpf, "d"_a)
    .def("get_zoom", &Camera::zoomValue)
    .def("zoom", &Camera::zoom, "delta"_a, "relative"_a)
    .def("get_fov", &Camera::fovValue)
    .def_rw("viewall", &Camera::viewall)
    .def_rw("autocenter", &Camera::autocenter)
    .def_rw("projection", &Camera::projection)
    .def_rw("pixel_width", &Camera::pixel_width)
    .def_rw("pixel_height", &Camera::pixel_height)
    .def_rw("locked", &Camera::locked)
    .def_rw("fov", &Camera::fov);

  nb::class_<ASTNode>(m, "ASTNode")
    .def("dump", &ASTNode::dump, "indent"_a = "", "Dump Node.")
    .def("location", &ASTNode::location, "Location.");

  nb::class_<Annotation>(m, "Annotation")
    .def("get_name", &Annotation::getName)
    .def("get_expr", &Annotation::getExpr);
    // .def("print");

  nb::class_<Expression, ASTNode>(m, "Expression")
    .def("is_literal", &Expression::isLiteral);

  nb::class_<SourceFile, ASTNode>(m, "SourceFile")
    .def_ro("scope", &SourceFile::scope)
    .def_ro("used_libraries", &SourceFile::usedlibs)
    .def_ro("indicator_data", &SourceFile::indicatorData)
    // .def(nb::init<std::string, std::string>())
    // .def("instantiate", "context"_a, "resulting_file_context"_a, "Instantiate")
    .def("register_use", &SourceFile::registerUse, "path"_a, "loc"_a, "Register Use of a library - use <lib.scad>")
    .def("get_fullpath", &SourceFile::getFullpath, "Get full path of source file.")
    .def("get_filename", &SourceFile::getFilename, "Get file name of the source file.")
    .def("has_includes", &SourceFile::hasIncludes, "Has includes")
    .def("is_handling_dependencies", &SourceFile::isHandlingDependencies, "Is handling dependencies")
    .def("get_module_path", &SourceFile::modulePath, "Get module path")
    .def("uses_libraries", &SourceFile::usesLibraries, "Uses libraries");
  
  nb::class_<LocalScope>(m, "LocalScope")
    .def_ro("assignments", &LocalScope::assignments)
    .def_ro("moduleInstantiations", &LocalScope::moduleInstantiations)
    .def_ro("functions", &LocalScope::functions)
    .def_ro("astFunctions", &LocalScope::astFunctions)
    .def_ro("modules", &LocalScope::modules)
    .def_ro("astModules", &LocalScope::astModules)
    .def("has_children", &LocalScope::hasChildren, "Has Children.")
    .def("num_elements", &LocalScope::numElements, "Number of Elements.")
    .def("print", &LocalScope::printss)
    // .def("printss", &LocalScope::print, "Print.")
    ;

  
    // .def("print", );
    // .def("set_location", &ASTNode::setLocation, "loc"_a, "Set Location")

  nb::class_<UserFunction, ASTNode>(m, "UserFunction");
  
  nb::class_<AbstractModule>(m, "AbstractModule")
    .def("is_experimental", &AbstractModule::is_experimental, "Is Experimental.")
    .def("is_enabled", &AbstractModule::is_enabled, "Is Enabled.");

  nb::class_<BuiltinModule, AbstractModule>(m, "AbstractModule");

  nb::class_<UserModule, AbstractModule>(m, "UserModule")
    .def("print", &UserModule::print)
    .def("dump", &UserModule::dump)
    .def("stack_size", &UserModule::stack_size)
    ; //, ASTNode
  
  nb::class_<ModuleInstantiation>(m, "ModuleInstantiation")
    .def("name", &ModuleInstantiation::name, "Name.")
    .def("is_background", &ModuleInstantiation::isBackground, "Is Background.")
    .def("is_highlight", &ModuleInstantiation::isHighlight, "Is Highlight.")
    .def("is_root", &ModuleInstantiation::isRoot, "Is Root.")
    .def_ro("arguments", &ModuleInstantiation::arguments, "Arguments")
    .def_ro("scope", &ModuleInstantiation::scope, "Scope");

  nb::class_<Assignment, ASTNode>(m, "Assignment")
    .def("get_expr", &Assignment::getExpr)
    .def("get_annotations", &Assignment::getAnnotations)
    .def("has_annotations", &Assignment::hasAnnotations)
    .def("get_name", &Assignment::getName);
    // .def("get_name", &Assignment::get_name, "Get name.");

  // nb::class_<AssignmentList>(m, "Assignment List");
  
  nb::class_<Location>(m, "Location")
    .def("file_name", &Location::fileName, "File name.")
    .def("file_path", &Location::filePath, "File path.")
    .def("first_line", &Location::firstLine, "First line.")
    .def("first_column", &Location::firstColumn, "First column.")
    .def("last_line", &Location::lastLine, "Last line.")
    .def("last_column", &Location::lastColumn, "Last column.")
    .def("is_none", &Location::isNone, "Is None.")
    .def("to_relative_string", &Location::toRelativeString, "To Relative String.")
    .def(nb::self == nb::self)
    .def(nb::self != nb::self);

  nb::class_<AbstractNode>(m, "AbstractNode")
    .def("__repr__", &AbstractNode::toString, "AbstractNode to string.")
    .def("name", &AbstractNode::name, "Name.")
    .def("verbose_name", &AbstractNode::verbose_name, "Verbose Name.")
    .def("get_children", &AbstractNode::getChildren, "Get Children.")
    .def("index", &AbstractNode::index, "Index.")
    .def("get_node_by_id", &AbstractNode::getNodeByID, "Get Node by Id.")
    .def_ro("modinst", &AbstractNode::modinst, "Index.")
    .def("set_debug", &AbstractNode::set_debug, "modifier"_a, "Set debug modifier - # (highlight) , ! (root), % (background), * (disable). ")
    .def("clear_debug", &AbstractNode::clear_debug, "Clear Debug.")
    .def("get_debug", &AbstractNode::get_debug, "Clear Debug.")
    .def("has_debug", &AbstractNode::has_debug, "Has Debug.")
    ;

  nb::class_<LeafNode, AbstractNode>(m, "LeafNode")
    .def("create_geometry", &LeafNode::createGeometry, "Create geometry.");

  nb::class_<AbstractPolyNode, AbstractNode>(m, "AbstractPolyNode");

  nb::class_<Geometry>(m, "Geometry")
    .def("memsize", &Geometry::memsize, "Memory size.")
    .def("get_bounding_box", &Geometry::getBoundingBox, "Get bounding box.")
    .def("dump", &Geometry::dump, "Dump Geometry.")
    .def("get_dimension", &Geometry::getDimension, "Get dimension.")
    .def("is_empty", &Geometry::isEmpty, "Is empty.")
    .def("num_facets", &Geometry::numFacets, "Number of facets.")
    .def("get_convexity", &Geometry::getConvexity, "Get convexity.")
    ;

  nb::class_<BoundingBox>(m, "BoundingBox")
    .def("volume", &BoundingBox::volume, "Volume");

  nb::class_<OSObject>(m, "OSObject")
    .def_ro("node", &OSObject::node, "node")
    .def_ro("transformations", &OSObject::transformations, "transformations");

  nb::class_<Primitive3D, OSObject>(m, "Primitive3D")
    .def("scale", nb::overload_cast<double>(&Primitive3D::scale), "size"_a, "Scale")
    .def("scale", nb::overload_cast<std::vector<double>&>(&Primitive3D::scale), "vec"_a, "Scale")
    // .def("color", nb::overload_cast<Color4f*>(&Primitive3D::color), "color"_a, "color")
    .def("color", nb::overload_cast<std::string>(&Primitive3D::color), "name"_a, "color")
    .def("color", nb::overload_cast<std::string, float>(&Primitive3D::color), "name"_a, "alpha"_a, "color")
    .def("color", nb::overload_cast<int, int, int>(&Primitive3D::color), "color")
    .def("color", nb::overload_cast<int, int, int, int>(&Primitive3D::color), "color")
    .def("color", nb::overload_cast<float, float, float>(&Primitive3D::color), "color")
    .def("color", nb::overload_cast<float, float, float, float>(&Primitive3D::color), "color")
    .def("translate", nb::overload_cast<std::vector<double>&>(&Primitive3D::translate), "vec"_a, "translate")
    .def("rotate", nb::overload_cast<std::vector<double>&>(&Primitive3D::rotate), "vec"_a, "rotate")
    .def("rotate", nb::overload_cast<double, std::vector<double>&>(&Primitive3D::rotate), "angle"_a, "vec"_a, "rotate")
    .def("rotate", nb::overload_cast<std::vector<double>&>(&Primitive3D::rotate), "angle"_a, "rotate")
    .def("mirror", nb::overload_cast<std::vector<double>&>(&Primitive3D::mirror), "vec"_a, "mirror")
    .def("multmatrix", nb::overload_cast<std::vector<std::vector<double>>&>(&Primitive3D::multmatrix), "vec"_a, "multmatrix")
    .def("union", nb::overload_cast<Primitive3D*>(&Primitive3D::union_), "primitive"_a, "union")
    .def("intersection", nb::overload_cast<Primitive3D*>(&Primitive3D::intersection), "primitive"_a, "intersection")
    .def("difference", nb::overload_cast<Primitive3D*>(&Primitive3D::difference), "primitive"_a, "difference")
    .def("minkowski", nb::overload_cast<>(&Primitive3D::minkowski), "minkowski")
    .def("minkowski", nb::overload_cast<int>(&Primitive3D::minkowski), "convexity"_a, "minkowski")
    .def("hull", nb::overload_cast<>(&Primitive3D::hull), "hull")
    .def("fill", nb::overload_cast<>(&Primitive3D::fill), "fill")
    .def("resize", nb::overload_cast<std::vector<double>&>(&Primitive3D::resize), "newsize"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, bool>(&Primitive3D::resize), "newsize"_a,  "autosize"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, bool, int>(&Primitive3D::resize), "newsize"_a, "autosize"_a, "convexity"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, std::vector<bool>&>(&Primitive3D::resize), "newsize"_a, "autosize"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, std::vector<bool>&, int>(&Primitive3D::resize), "newsize"_a,  "autosize"_a, "convexity"_a, "resize");

  nb::class_<Primitive2D, OSObject>(m, "Primitive2D")
    .def("scale", nb::overload_cast<double>(&Primitive2D::scale), "size"_a, "Scale")
    .def("scale", nb::overload_cast<std::vector<double>&>(&Primitive2D::scale), "vec"_a, "Scale")
    // .def("color", nb::overload_cast<Color4f*>(&Primitive2D::color), "color"_a, "color")
    .def("color", nb::overload_cast<std::string>(&Primitive2D::color), "name"_a, "color")
    .def("color", nb::overload_cast<std::string, float>(&Primitive2D::color), "name"_a, "alpha"_a, "color")
    .def("color", nb::overload_cast<int, int, int>(&Primitive2D::color), "color")
    .def("color", nb::overload_cast<int, int, int, int>(&Primitive2D::color), "color")
    .def("color", nb::overload_cast<float, float, float>(&Primitive2D::color), "color")
    .def("color", nb::overload_cast<float, float, float, float>(&Primitive2D::color), "color")
    .def("translate", nb::overload_cast<std::vector<double>&>(&Primitive2D::translate), "vec"_a, "translate")
    .def("rotate", nb::overload_cast<std::vector<double>&>(&Primitive2D::rotate), "vec"_a, "rotate")
    .def("rotate", nb::overload_cast<double, std::vector<double>&>(&Primitive2D::rotate), "angle"_a, "vec"_a, "rotate")
    .def("rotate", nb::overload_cast<std::vector<double>&>(&Primitive2D::rotate), "angle"_a, "rotate")
    .def("mirror", nb::overload_cast<std::vector<double>&>(&Primitive2D::mirror), "vec"_a, "mirror")
    .def("multmatrix", nb::overload_cast<std::vector<std::vector<double>>&>(&Primitive2D::multmatrix), "vec"_a, "multmatrix")
    .def("union", nb::overload_cast<Primitive2D*>(&Primitive2D::union_), "primitive"_a, "union")
    .def("intersection", nb::overload_cast<Primitive2D*>(&Primitive2D::intersection), "primitive"_a, "intersection")
    .def("difference", nb::overload_cast<Primitive2D*>(&Primitive2D::difference), "primitive"_a, "difference")
    .def("minkowski", nb::overload_cast<>(&Primitive2D::minkowski), "minkowski")
    .def("minkowski", nb::overload_cast<int>(&Primitive2D::minkowski), "convexity"_a, "minkowski")
    .def("hull", nb::overload_cast<>(&Primitive2D::hull), "hull")
    .def("fill", nb::overload_cast<>(&Primitive2D::fill), "fill")
    .def("resize", nb::overload_cast<std::vector<double>&>(&Primitive2D::resize), "newsize"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, bool>(&Primitive2D::resize), "newsize"_a,  "autosize"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, bool, int>(&Primitive2D::resize), "newsize"_a, "autosize"_a, "convexity"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, std::vector<bool>&>(&Primitive2D::resize), "newsize"_a, "autosize"_a, "resize")
    .def("resize", nb::overload_cast<std::vector<double>&, std::vector<bool>&, int>(&Primitive2D::resize), "newsize"_a,  "autosize"_a, "convexity"_a, "resize")
    .def("linear_extrude", nb::overload_cast<double>(&Primitive2D::linear_extrude), "height"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, bool>(&Primitive2D::linear_extrude),  "height"_a, "center"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, double>(&Primitive2D::linear_extrude),  "height"_a,  "twist"_a)
    .def("linear_extrude", nb::overload_cast<double, int, double>(&Primitive2D::linear_extrude), "height"_a, "convexity"_a, "twist"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, int, double, double>(&Primitive2D::linear_extrude), "height"_a, "convexity"_a, "twist"_a, "scale"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, int, double, std::vector<double>&>(&Primitive2D::linear_extrude), "height"_a, "convexity"_a, "twist"_a, "scale"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, bool, int, double>(&Primitive2D::linear_extrude),  "height"_a, "center"_a, "convexity"_a, "twist"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, bool, int, double, double>(&Primitive2D::linear_extrude), "height"_a, "center"_a,  "convexity"_a,  "twist"_a, "scale"_a, "linear extrude")
    .def("linear_extrude", nb::overload_cast<double, bool, int, double, std::vector<double>&>(&Primitive2D::linear_extrude), "height"_a, "center"_a,  "convexity"_a,  "twist"_a, "scale"_a, "linear extrude")
    .def("rotate_extrude", nb::overload_cast<double>(&Primitive2D::rotate_extrude), "angle"_a, "rotate extrude")
    .def("rotate_extrude", nb::overload_cast<double, int>(&Primitive2D::rotate_extrude), "angle"_a, "convexity"_a, "rotate extrude")
    .def("offset", nb::overload_cast<std::string, double>(&Primitive2D::offset), "op"_a, "delta"_a, "offset")
    .def("offset", nb::overload_cast<std::string, double, bool>(&Primitive2D::offset), "op"_a, "delta"_a, "chamfer"_a, "offset")
    ;

  m.def("cube", nb::overload_cast<>(&Primitive3D::cube), "Create Cube");
  m.def("cube", nb::overload_cast<double>(&Primitive3D::cube), "size"_a, "Create Cube");
  m.def("cube", nb::overload_cast<double, bool>(&Primitive3D::cube), "size"_a, "center"_a, "Create Cube");
  m.def("cube", nb::overload_cast<double, double, double>(&Primitive3D::cube), "x"_a, "y"_a, "z"_a, "Create Cube");
  m.def("cube", nb::overload_cast<double, double, double, bool>(&Primitive3D::cube), "x"_a, "y"_a, "z"_a, "center"_a, "Create Cube");

  m.def("sphere", nb::overload_cast<>(&Primitive3D::sphere), "Create Sphere");
  m.def("sphere", nb::overload_cast<double>(&Primitive3D::sphere), "radius"_a, "Create Sphere");

  m.def("cylinder", nb::overload_cast<>(&Primitive3D::cylinder), "Create Cylinder");
  m.def("cylinder", nb::overload_cast<double, double>(&Primitive3D::cylinder), "radius"_a, "height"_a, "Create Cylinder");
  m.def("cylinder", nb::overload_cast<double, double, bool>(&Primitive3D::cylinder), "radius"_a, "height"_a, "center"_a, "Create Cylinder");
  m.def("cylinder", nb::overload_cast<double, double, double>(&Primitive3D::cylinder), "radius1"_a, "radius2"_a, "height"_a, "Create Cylinder");
  m.def("cylinder", nb::overload_cast<double, double, double, bool>(&Primitive3D::cylinder), "radius1"_a, "radius2"_a, "height"_a, "center"_a, "Create Cylinder");

  m.def("polyhedron", nb::overload_cast<>(&Primitive3D::polyhedron), "Create Polyhedron");
  m.def("polyhedron", nb::overload_cast<std::vector<point3d>&, std::vector<std::vector<size_t>>&>(&Primitive3D::polyhedron), "points"_a, "faces"_a, "Create Polyhedron");
  m.def("polyhedron", nb::overload_cast<std::vector<point3d>&, std::vector<std::vector<size_t>>&, int>(&Primitive3D::polyhedron), "points"_a, "faces"_a, "convexity"_a, "Create Polyhedron");

  m.def("square", nb::overload_cast<>(&Primitive2D::square), "Create Square");
  m.def("square", nb::overload_cast<double>(&Primitive2D::square), "size"_a, "Create Square");
  m.def("square", nb::overload_cast<double, bool>(&Primitive2D::square), "size"_a, "center"_a, "Create Square");
  m.def("square", nb::overload_cast<double, double>(&Primitive2D::square), "x"_a, "y"_a, "Create Square");
  m.def("square", nb::overload_cast<double, double, bool>(&Primitive2D::square), "x"_a, "y"_a, "center"_a, "Create Square");

  m.def("circle", nb::overload_cast<>(&Primitive2D::circle), "Create Circle");
  m.def("circle", nb::overload_cast<double>(&Primitive2D::circle), "radius"_a, "Create Circle");

  // m.def("polygon", nb::overload_cast<>(&Primitive2D::polygon), "Create Polygon");
  m.def("polygon", nb::overload_cast<std::vector<point2d>&, std::vector<std::vector<size_t>>&>(&Primitive2D::polygon), "points"_a, "faces"_a, "Create Polygon");
  m.def("polygon", nb::overload_cast<std::vector<point2d>&, std::vector<std::vector<size_t>>&, int>(&Primitive2D::polygon), "points"_a, "faces"_a, "convexity"_a, "Create Polygon");

  // m.def("text", nb::overload_cast<>(&Primitive2D::text), "Create Text");
  m.def("text", nb::overload_cast<FreetypeRenderer::Params&>(&Primitive2D::text), "Create Text");
  m.def("text", nb::overload_cast<std::string&>(&Primitive2D::text), "Create Text");
  m.def("text", nb::overload_cast<std::string&, int>(&Primitive2D::text), "Create Text");
  m.def("text", nb::overload_cast<std::string&, int, std::string&>(&Primitive2D::text), "Create Text");

  nb::class_<CubeNode, LeafNode>(m, "Cube")
    .def(nb::init<>())
    .def(nb::init<double>())
    .def(nb::init<double, bool>())
    .def(nb::init<double, double, double>())
    .def(nb::init<double, double, double, bool>())
    .def_rw("center", &CubeNode::center)
    .def_rw("x", &CubeNode::x)
    .def_rw("y", &CubeNode::y)
    .def_rw("z", &CubeNode::z);

  nb::class_<SphereNode, LeafNode>(m, "Sphere")
    .def(nb::init<>())
    .def(nb::init<double>())
    .def_rw("r", &SphereNode::r)
    .def_rw("fn", &SphereNode::fn)
    .def_rw("fs", &SphereNode::fs)
    .def_rw("fa", &SphereNode::fa);

  nb::class_<CylinderNode, LeafNode>(m, "Cylinder")
    .def(nb::init<>())
    .def(nb::init<double, double>())
    .def(nb::init<double, double, bool>())
    .def(nb::init<double, double, double>())
    .def(nb::init<double, double, double, bool>())
    .def_rw("center", &CylinderNode::center)
    .def_rw("h", &CylinderNode::h)
    .def_rw("r1", &CylinderNode::r1)
    .def_rw("r2", &CylinderNode::r2)
    .def_rw("fn", &CylinderNode::fn)
    .def_rw("fs", &CylinderNode::fs)
    .def_rw("fa", &CylinderNode::fa);

  nb::class_<PolyhedronNode, LeafNode>(m, "Polyhedron")
    .def(nb::init<>())
    .def(nb::init<std::vector<point3d>&, std::vector<std::vector<size_t>>&>())
    .def(nb::init<std::vector<point3d>&, std::vector<std::vector<size_t>>&, int>())
    .def_rw("points", &PolyhedronNode::points)
    .def_rw("faces", &PolyhedronNode::faces)
    .def_rw("convexity", &PolyhedronNode::convexity);

  nb::class_<SquareNode, LeafNode>(m, "Square")
    .def(nb::init<>())
    .def(nb::init<double>())
    .def(nb::init<double, bool>())
    .def(nb::init<double, double>())
    .def(nb::init<double, double, bool>())
    .def_rw("center", &SquareNode::center)
    .def_rw("x", &SquareNode::x)
    .def_rw("y", &SquareNode::y);

  nb::class_<CircleNode, LeafNode>(m, "Circle")
    .def(nb::init<>())
    .def(nb::init<double>())
    .def_rw("r", &CircleNode::r)
    .def_rw("fn", &CircleNode::fn)
    .def_rw("fs", &CircleNode::fs)
    .def_rw("fa", &CircleNode::fa);

  nb::class_<PolygonNode, LeafNode>(m, "Polygon")
    .def(nb::init<>())
    .def(nb::init<std::vector<point2d>&, std::vector<std::vector<size_t>>&>())
    .def(nb::init<std::vector<point2d>&, std::vector<std::vector<size_t>>&, int>())
    .def_rw("points", &PolygonNode::points)
    .def_rw("paths", &PolygonNode::paths)
    .def_rw("convexity", &PolygonNode::convexity);

  nb::class_<SurfaceNode, LeafNode>(m, "Surface")
    .def(nb::init<>())
    .def(nb::init<std::string>())
    .def(nb::init<std::string, bool>())
    .def(nb::init<std::string, bool, int>())
    .def(nb::init<std::string, bool, bool>())
    .def(nb::init<std::string, bool, bool, int>())
    .def_rw("convexity", &SurfaceNode::convexity)
    .def_rw("center", &SurfaceNode::center)
    .def_rw("invert", &SurfaceNode::invert)
    .def_rw("file", &SurfaceNode::filename);

  nb::class_<Color4f>(m, "Color4f")
    .def(nb::init<>())
    .def(nb::init<int, int, int>())
    .def(nb::init<int, int, int, int>())
    .def(nb::init<float, float, float>())
    .def(nb::init<float, float, float, float>())
    .def("set_rgb", &Color4f::setRgb)
    .def("is_valid", &Color4f::isValid);

  nb::class_<ColorNode, AbstractNode>(m, "Color")
    .def(nb::init<>())
    .def(nb::init<Color4f*>())
    .def(nb::init<std::string>())
    .def(nb::init<std::string, float>())
    .def(nb::init<int, int, int>())
    .def(nb::init<int, int, int, int>())
    .def(nb::init<float, float, float>())
    .def(nb::init<float, float, float, float>())
    .def_rw("color", &ColorNode::color)
    .def_static("get_web_colors", &ColorNode::getWebColors);

  nb::class_<FreetypeRenderer::Params>(m, "TextParams")
    .def(nb::init<>())
    .def_rw("size", &FreetypeRenderer::Params::size)
    .def_rw("spacing", &FreetypeRenderer::Params::spacing)
    .def_rw("fn", &FreetypeRenderer::Params::fn)
    .def_rw("fa", &FreetypeRenderer::Params::fa)
    .def_rw("fs", &FreetypeRenderer::Params::fs)
    .def_rw("segments", &FreetypeRenderer::Params::segments)
    .def_rw("text", &FreetypeRenderer::Params::text)
    .def_rw("font", &FreetypeRenderer::Params::font)
    .def_rw("direction", &FreetypeRenderer::Params::direction)
    .def_rw("language", &FreetypeRenderer::Params::language)
    .def_rw("script", &FreetypeRenderer::Params::script)
    .def_rw("halign", &FreetypeRenderer::Params::halign)
    .def_rw("valign", &FreetypeRenderer::Params::valign)
    .def_rw("loc", &FreetypeRenderer::Params::loc)
    .def_rw("documentPath", &FreetypeRenderer::Params::documentPath);
  
  nb::class_<TextNode, AbstractPolyNode>(m, "Text")
    .def(nb::init<>())
    .def(nb::init<FreetypeRenderer::Params&>())
    .def(nb::init<std::string&>())
    .def(nb::init<std::string&, int>())
    .def(nb::init<std::string&, int, std::string&>())
    .def("create_geometry_list", &TextNode::createGeometryList)
    .def_rw("params", &TextNode::params);

  nb::class_<GroupNode, AbstractNode>(m, "Group")
    .def(nb::init<>())
    .def(nb::init<std::string>());

  nb::class_<RootNode, GroupNode>(m, "Root")
    .def(nb::init<>());

  nb::class_<TransformNode, AbstractNode>(m, "Transform")
    .def(nb::init<std::string>())
    .def_static("scale", nb::overload_cast<double>(&TransformNode::scale))
    .def_static("scale", nb::overload_cast<std::vector<double>&>(&TransformNode::scale))
    .def_static("rotate", nb::overload_cast<std::vector<double>&>(&TransformNode::rotate))
    .def_static("rotate", nb::overload_cast<double, std::vector<double>&>(&TransformNode::rotate))
    .def_static("rotate", nb::overload_cast<double>(&TransformNode::rotate))
    .def_static("translate", nb::overload_cast<std::vector<double>&>(&TransformNode::translate))
    .def_static("mirror", nb::overload_cast<std::vector<double>&>(&TransformNode::mirror))
    .def_static("multmatrix", nb::overload_cast<std::vector<std::vector<double>>&>(&TransformNode::multmatrix))
    .def_rw("matrix", &TransformNode::matrix);

  nb::class_<CsgOpNode, AbstractNode>(m, "TransformCsgOp")
    .def_static("union", &CsgOpNode::union_)
    .def_static("intersection", &CsgOpNode::intersection)
    .def_static("difference", &CsgOpNode::difference);

  nb::class_<CgalAdvNode, AbstractNode>(m, "TransformCgalAdvance")
    .def_static("minkowski", &CgalAdvNode::minkowski)
    .def_static("hull", &CgalAdvNode::hull)
    .def_static("fill", &CgalAdvNode::fill)
    .def_static("resize", nb::overload_cast<std::vector<double>&>(&CgalAdvNode::resize))
    .def_static("resize", nb::overload_cast<std::vector<double>&, bool>(&CgalAdvNode::resize))
    .def_static("resize", nb::overload_cast<std::vector<double>&, std::vector<bool>&>(&CgalAdvNode::resize))
    .def_static("resize", nb::overload_cast<std::vector<double>&, bool, int>(&CgalAdvNode::resize))
    .def_static("resize", nb::overload_cast<std::vector<double>&, std::vector<bool>&, int>(&CgalAdvNode::resize))
    ;

  nb::class_<OffsetNode, AbstractPolyNode>(m, "Offset")
    .def(nb::init<>())
    .def(nb::init<std::string, double>())
    .def(nb::init<std::string, double, bool>())
    .def_rw("delta", &OffsetNode::delta)
    .def_rw("chamfer", &OffsetNode::chamfer)
    .def_rw("join_type", &OffsetNode::join_type)
    .def_rw("fn", &OffsetNode::fn)
    .def_rw("fs", &OffsetNode::fs)
    .def_rw("fa", &OffsetNode::fa);

  nb::class_<ProjectionNode, AbstractPolyNode>(m, "Projection")
    .def(nb::init<>())
    .def(nb::init<bool>())
    .def_rw("convexity", &ProjectionNode::convexity)
    .def_rw("cut_mode", &ProjectionNode::cut_mode);

  nb::class_<QuotedString>(m, "QuotedString")
    .def(nb::init<>())
    .def(nb::init<std::string&>())
    .def("__repr__", [](const QuotedString &f) { 
      std::ostringstream stream;
      stream << "<QuotedString: " << f << ">"; 
      return stream.str();
    });

  nb::class_<Filename, QuotedString>(m, "Filename") //, QuotedString
    .def(nb::init<>())
    .def(nb::init<std::string&>())
    .def("__repr__", [](const Filename &f) { 
      std::ostringstream stream;
      stream << "<Filename: " << f << ">"; 
      return stream.str();
    });

  nb::class_<RoofNode, AbstractPolyNode>(m, "Roof")
    .def(nb::init<>())
    .def(nb::init<int>())
    .def(nb::init<std::string>())
    .def(nb::init<std::string, int>())
    .def_rw("method", &RoofNode::method)
    .def_rw("convexity", &RoofNode::convexity)
    .def_rw("fn", &RoofNode::fn)
    .def_rw("fs", &RoofNode::fs)
    .def_rw("fa", &RoofNode::fa);

  nb::class_<LinearExtrudeNode, AbstractPolyNode>(m, "LinearExtrude")
    .def(nb::init<>())
    .def(nb::init<double>())
    .def(nb::init<double, bool>())
    .def(nb::init<double, double>())
    .def(nb::init<double, int, double>())
    .def(nb::init<double, int, double>())
    .def(nb::init<double, int, double, double>())
    .def(nb::init<double, int, double, std::vector<double>&>())
    .def(nb::init<double, bool, int, double, double>())
    .def(nb::init<double, bool, int, double, std::vector<double>&>())
    .def_rw("height", &LinearExtrudeNode::height)
    .def_rw("scale_x", &LinearExtrudeNode::scale_x)
    .def_rw("scale_y", &LinearExtrudeNode::scale_y)
    .def_rw("center", &LinearExtrudeNode::center)
    .def_rw("convexity", &LinearExtrudeNode::convexity)
    .def_prop_rw("twist", 
      [](LinearExtrudeNode &le) { return le.twist ; },
      [](LinearExtrudeNode &le, double value) { le.set_twist(value); })
    .def_prop_rw("slices", 
      [](LinearExtrudeNode &le) { return le.slices ; },
      [](LinearExtrudeNode &le, unsigned int value) { le.set_slices(value); })
    .def_prop_rw("segments", 
      [](LinearExtrudeNode &le) { return le.segments ; },
      [](LinearExtrudeNode &le, unsigned int value) { le.set_segments(value); })
    .def_rw("fn", &LinearExtrudeNode::fn)
    .def_rw("fs", &LinearExtrudeNode::fs)
    .def_rw("fa", &LinearExtrudeNode::fa);

  nb::class_<RotateExtrudeNode, AbstractPolyNode>(m, "RotateExtrude")
    .def(nb::init<>())
    .def(nb::init<double>())
    .def(nb::init<double, int>())
    .def_rw("angle", &RotateExtrudeNode::angle)
    .def_rw("convexity", &RotateExtrudeNode::convexity)
    .def_rw("fn", &RotateExtrudeNode::fn)
    .def_rw("fs", &RotateExtrudeNode::fs)
    .def_rw("fa", &RotateExtrudeNode::fa);
     
  nb::class_<RenderNode, AbstractNode>(m, "Render")
    .def(nb::init<>())
    .def(nb::init<int>())
    .def_rw("convexity", &RenderNode::convexity);

}


  // m.def("cube", nb::overload_cast<>(&CubeNode::cube), "Create Cube");
  // m.def("cube", nb::overload_cast<double>(&CubeNode::cube), "size"_a, "Create Cube");
  // m.def("cube", nb::overload_cast<double, bool>(&CubeNode::cube), "size"_a, "center"_a, "Create Cube");
  // m.def("cube", nb::overload_cast<double, double, double>(&CubeNode::cube), "x"_a, "y"_a, "z"_a, "Create Cube");
  // m.def("cube", nb::overload_cast<double, double, double, bool>(&CubeNode::cube), "x"_a, "y"_a, "z"_a, "center"_a, "Create Cube");

  // m.def("sphere", nb::overload_cast<>(&SphereNode::sphere), "Create Sphere");
  // m.def("sphere", nb::overload_cast<double>(&SphereNode::sphere), "radius"_a, "Create Sphere");

  // m.def("cylinder", nb::overload_cast<>(&CylinderNode::cylinder), "Create Cylinder");
  // m.def("cylinder", nb::overload_cast<double, double>(&CylinderNode::cylinder), "radius"_a, "height"_a, "Create Cylinder");
  // m.def("cylinder", nb::overload_cast<double, double, bool>(&CylinderNode::cylinder), "radius"_a, "height"_a, "center"_a, "Create Cylinder");
  // m.def("cylinder", nb::overload_cast<double, double, double>(&CylinderNode::cylinder), "radius1"_a, "radius2"_a, "height"_a, "Create Cylinder");
  // m.def("cylinder", nb::overload_cast<double, double, double, bool>(&CylinderNode::cylinder), "radius1"_a, "radius2"_a, "height"_a, "center"_a, "Create Cylinder");

  // m.def("polyhedron", nb::overload_cast<>(&PolyhedronNode::polyhedron), "Create Polyhedron");
  // m.def("polyhedron", nb::overload_cast<std::vector<point3d>&, std::vector<std::vector<size_t>>&>(&PolyhedronNode::polyhedron), "points"_a, "faces"_a, "Create Polyhedron");
  // m.def("polyhedron", nb::overload_cast<std::vector<point3d>&, std::vector<std::vector<size_t>>&, int>(&PolyhedronNode::polyhedron), "points"_a, "faces"_a, "convexity"_a, "Create Polyhedron");


  // m.def("square", nb::overload_cast<>(&SquareNode::square), "Create Square");
  // m.def("square", nb::overload_cast<double>(&SquareNode::square), "size"_a, "Create Square");
  // m.def("square", nb::overload_cast<double, bool>(&SquareNode::square), "size"_a, "center"_a, "Create Square");
  // m.def("square", nb::overload_cast<double, double>(&SquareNode::square), "x"_a, "y"_a, "Create Square");
  // m.def("square", nb::overload_cast<double, double, bool>(&SquareNode::square), "x"_a, "y"_a, "center"_a, "Create Square");

  // m.def("circle", nb::overload_cast<>(&CircleNode::circle), "Create Circle");
  // m.def("circle", nb::overload_cast<double>(&CircleNode::circle), "radius"_a, "Create Circle");

  // m.def("polygon", nb::overload_cast<>(&PolygonNode::polygon), "Create Polygon");
  // m.def("polygon", nb::overload_cast<std::vector<point2d>&, std::vector<std::vector<size_t>>&>(&PolygonNode::polygon), "points"_a, "faces"_a, "Create Polygon");
  // m.def("polygon", nb::overload_cast<std::vector<point2d>&, std::vector<std::vector<size_t>>&, int>(&PolygonNode::polygon), "points"_a, "faces"_a, "convexity"_a, "Create Polygon");

  // m.def("text", nb::overload_cast<>(&TextNode::text), "Create Text");
  // m.def("text", nb::overload_cast<FreetypeRenderer::Params&>(&TextNode::text), "Create Text");
  // m.def("text", nb::overload_cast<std::string&>(&TextNode::text), "Create Text");
  // m.def("text", nb::overload_cast<std::string&, int>(&TextNode::text), "Create Text");
  // m.def("text", nb::overload_cast<std::string&, int, std::string&>(&TextNode::text), "Create Text");

  // m.def("color", nb::overload_cast<>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<Color4f*>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<std::string>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<std::string, float>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<int, int, int>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<int, int, int, int>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<float, float, float>(&ColorNode::color_), "Create Color");
  // m.def("color", nb::overload_cast<float, float, float, float>(&ColorNode::color_), "Create Color");


  // m.def("scale", nb::overload_cast<double>(&TransformNode::scale), "Scale")
  // m.def("scale", nb::overload_cast<std::vector<double>&>(&TransformNode::scale))
  // m.def("rotate", nb::overload_cast<std::vector<double>&>(&TransformNode::rotate))
  // m.def("rotate", nb::overload_cast<double, std::vector<double>&>(&TransformNode::rotate))
  // m.def("rotate", nb::overload_cast<double>(&TransformNode::rotate))
  // m.def("translate", nb::overload_cast<std::vector<double>&>(&TransformNode::translate))
  // m.def("mirror", nb::overload_cast<std::vector<double>&>(&TransformNode::mirror))
  // m.def("multmatrix", nb::overload_cast<std::vector<std::vector<double>>&>(&TransformNode::multmatrix))

#ifndef PYFUNCTIONS_H
#define PYFUNCTIONS_H

#include <Python.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
class AbstractNode;
class Tree;
class Geometry;
class PolySet;
class Export3mfPartInfo;
struct SphereEdgeDb;
enum class OpenSCADOperator;
enum class CgalAdvType;
enum class ImportType;

// Python type objects
extern PyTypeObject PyOpenSCADType;
extern PyTypeObject PyDataType;

// Global variables
extern PyObject *python_result_obj;
extern std::vector<std::shared_ptr<AbstractNode>> shows;
extern std::shared_ptr<AbstractNode> void_node;
extern std::shared_ptr<AbstractNode> full_node;
extern std::shared_ptr<AbstractNode> genlang_result_node;

// Primitive creation functions
PyObject *python_edge(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_marked(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_cube(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_sphere(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_cylinder(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_polyhedron(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_square(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_circle(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_polygon(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_spline(PyObject *self, PyObject *args, PyObject *kwargs);

#ifdef ENABLE_LIBFIVE
PyObject *python_frep(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_ifrep(PyObject *self, PyObject *args, PyObject *kwargs);
#endif

// Transformation functions
PyObject *python_translate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_translate_core(PyObject *obj, PyObject *v);
PyObject *python_rotate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_rotate_core(PyObject *obj, PyObject *val_a, PyObject *val_v, PyObject *ref);
PyObject *python_scale(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_scale_core(PyObject *obj, PyObject *val_v);
PyObject *python_mirror(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_mirror_core(PyObject *obj, PyObject *val_v);
PyObject *python_multmatrix(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_divmatrix(PyObject *self, PyObject *args, PyObject *kwargs);

// Directional movement functions
PyObject *python_right(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_left(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_front(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_back(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_down(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_up(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_rotx(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_roty(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_rotz(PyObject *self, PyObject *args, PyObject *kwargs);

// Object-oriented method variants
PyObject *python_oo_translate(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_rotate(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_scale(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_mirror(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_multmatrix(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_divmatrix(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_right(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_left(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_front(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_back(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_down(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_up(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_rotx(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_roty(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_rotz(PyObject *self, PyObject *args, PyObject *kwargs);

// Math functions
PyObject *python_sin(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_cos(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_tan(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_asin(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_acos(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_atan(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_dot(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_cross(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_norm(PyObject *self, PyObject *args, PyObject *kwargs);

// CSG operations
PyObject *python_union(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_difference(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_intersection(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_hull(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_fill(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_minkowski(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_resize(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_concat(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_skin(PyObject *self, PyObject *args, PyObject *kwargs);

// Object-oriented CSG operations
PyObject *python_oo_union(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_difference(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_intersection(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_resize(PyObject *obj, PyObject *args, PyObject *kwargs);

// Extrusion functions
PyObject *python_linear_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_rotate_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_path_extrude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_linear_extrude(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_rotate_extrude(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_path_extrude(PyObject *obj, PyObject *args, PyObject *kwargs);

// Modifier functions
PyObject *python_color(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_color_core(PyObject *obj, PyObject *color, double alpha);
PyObject *python_oo_color(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_pull(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_pull(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_wrap(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_wrap(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_offset(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_offset(PyObject *obj, PyObject *args, PyObject *kwargs);

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
PyObject *python_roof(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_roof(PyObject *obj, PyObject *args, PyObject *kwargs);
#endif

// Debug and visualization functions
PyObject *python_show(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_show_core(PyObject *obj);
PyObject *python_oo_show(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_output(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_oo_output(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_highlight(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_background(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_only(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_highlight(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_background(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_only(PyObject *self, PyObject *args, PyObject *kwargs);

// Analysis functions
PyObject *python_mesh(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_mesh_core(PyObject *obj, bool tessellate);
PyObject *python_oo_mesh(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_bbox(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_bbox_core(PyObject *obj);
PyObject *python_oo_bbox(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_size(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_size_core(PyObject *obj);
PyObject *python_oo_size(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_separate(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_separate_core(PyObject *obj);
PyObject *python_oo_separate(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_faces(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_faces_core(PyObject *obj, bool tessellate);
PyObject *python_oo_faces(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_edges(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_edges_core(PyObject *obj);
PyObject *python_oo_edges(PyObject *obj, PyObject *args, PyObject *kwargs);

// Processing functions
PyObject *python_oversample(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_oversample(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_debug(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_debug(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_repair(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_repair(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_fillet(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_fillet(PyObject *obj, PyObject *args, PyObject *kwargs);

// Import/Export functions
PyObject *python_export(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_export_core(PyObject *obj, char *file);
PyObject *python_oo_export(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_import(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *do_import_python(PyObject *self, PyObject *args, PyObject *kwargs, ImportType type);

#ifndef OPENSCAD_NOGUI
PyObject *python_nimport(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_add_menuitem(PyObject *self, PyObject *args, PyObject *kwargs, int mode);
#endif

// Projection and rendering
PyObject *python_projection(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_projection_core(PyObject *obj, PyObject *cut, int convexity);
PyObject *python_oo_projection(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_render(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_render_core(PyObject *obj, int convexity);
PyObject *python_oo_render(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_surface(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_surface_core(const char *file, PyObject *center, PyObject *invert, PyObject *color,
                              int convexity);

// Text functions
PyObject *python_text(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_textmetrics(PyObject *self, PyObject *args, PyObject *kwargs);

// Utility functions
PyObject *python_find_face(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_find_face_core(PyObject *obj, PyObject *vec_p);
PyObject *python_oo_find_face(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_sitonto(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_sitonto_core(PyObject *pyobj, PyObject *vecx_p, PyObject *vecy_p, PyObject *vecz_p);
PyObject *python_oo_sitonto(PyObject *obj, PyObject *args, PyObject *kwargs);
PyObject *python_align(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_align_core(PyObject *obj, PyObject *pyrefmat, PyObject *pydstmat);
PyObject *python_oo_align(PyObject *obj, PyObject *args, PyObject *kwargs);

// System functions
PyObject *python_group(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_osversion(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_osversion_num(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_osuse(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_osinclude(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_add_parameter(PyObject *self, PyObject *args, PyObject *kwargs, ImportType type);
PyObject *python_scad(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_model(PyObject *self, PyObject *args, PyObject *kwargs, int mode);
PyObject *python_modelpath(PyObject *self, PyObject *args, PyObject *kwargs, int mode);

// Object methods
PyObject *python_oo_clone(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_oo_dict(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *python_str(PyObject *self);

// Dictionary access
PyObject *python__getitem__(PyObject *obj, PyObject *key);
int python__setitem__(PyObject *dict, PyObject *key, PyObject *v);

// Number operations
PyObject *python_nb_add(PyObject *arg1, PyObject *arg2);
PyObject *python_nb_subtract(PyObject *arg1, PyObject *arg2);
PyObject *python_nb_mul(PyObject *arg1, PyObject *arg2);
PyObject *python_nb_or(PyObject *arg1, PyObject *arg2);
PyObject *python_nb_and(PyObject *arg1, PyObject *arg2);
PyObject *python_nb_xor(PyObject *arg1, PyObject *arg2);
PyObject *python_nb_invert(PyObject *arg);
PyObject *python_nb_neg(PyObject *arg);
PyObject *python_nb_pos(PyObject *arg);

// Helper functions
PyObject *python_nb_sub(PyObject *arg1, PyObject *arg2, OpenSCADOperator mode);
PyObject *python_nb_sub_vec3(PyObject *arg1, PyObject *arg2, int mode);
PyObject *python_debug_modifier(PyObject *arg, int mode);

// Matrix and vector conversion functions
int python_tomatrix(PyObject *pyt, Matrix4d& mat);
int python_tovector(PyObject *pyt, Vector3d& vec);
PyObject *python_frommatrix(const Matrix4d& mat);
PyObject *python_fromvector(const Vector3d vec);

// Transform helper functions
PyObject *python_translate_sub(PyObject *obj, Vector3d translatevec, int dragflags);
PyObject *python_rotate_sub(PyObject *obj, Vector3d vec3, double angle, int dragflags);
PyObject *python_scale_sub(PyObject *obj, Vector3d scalevec);
PyObject *python_mirror_sub(PyObject *obj, Matrix4d& m);
PyObject *python_multmatrix_sub(PyObject *pyobj, PyObject *pymat, int div);

// Number transformation functions
PyObject *python_number_scale(PyObject *pynum, Vector3d scalevec, int vecs);
PyObject *python_number_rot(PyObject *mat, Matrix3d rotvec, int vecs);
PyObject *python_number_mirror(PyObject *mat, Matrix4d m, int vecs);
PyObject *python_number_trans(PyObject *pynum, Vector3d transvec, int vecs);

// Core functions for different operations
PyObject *python_resize_core(PyObject *obj, PyObject *newsize, PyObject *autosize, int convexity);
PyObject *python_pull_core(PyObject *obj, PyObject *anchor, PyObject *dir);
PyObject *python_wrap_core(PyObject *obj, PyObject *target, double r, double d, double fn, double fa,
                           double fs);
PyObject *python_offset_core(PyObject *obj, double r, double delta, PyObject *chamfer, double fn,
                             double fa, double fs);
PyObject *python_oversample_core(PyObject *obj, int n, PyObject *round);
PyObject *python_debug_core(PyObject *obj, PyObject *faces);
PyObject *python_repair_core(PyObject *obj, PyObject *color);
PyObject *python_fillet_core(PyObject *obj, double r, int fn, PyObject *sel, double minang);

#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
PyObject *python_roof_core(PyObject *obj, const char *method, int convexity, double fn, double fa,
                           double fs);
#endif

// Extrusion core functions
PyObject *rotate_extrude_core(PyObject *obj, int convexity, double scale, double angle, PyObject *twist,
                              PyObject *origin, PyObject *offset, PyObject *vp, char *method, double fn,
                              double fa, double fs);
PyObject *linear_extrude_core(PyObject *obj, PyObject *height, int convexity, PyObject *origin,
                              PyObject *scale, PyObject *center, int slices, int segments,
                              PyObject *twist, double fn, double fa, double fs);
PyObject *path_extrude_core(PyObject *obj, PyObject *path, PyObject *xdir, int convexity,
                            PyObject *origin, PyObject *scale, PyObject *twist, PyObject *closed,
                            PyObject *allow_intersect, double fn, double fa, double fs);

// CSG helper functions
PyObject *python_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode);
PyObject *python_oo_csg_sub(PyObject *self, PyObject *args, PyObject *kwargs, OpenSCADOperator mode);
PyObject *python_csg_adv_sub(PyObject *self, PyObject *args, PyObject *kwargs, CgalAdvType mode);

// Directional movement helpers
PyObject *python_dir_sub(PyObject *self, PyObject *args, PyObject *kwargs, int mode);
PyObject *python_oo_dir_sub(PyObject *obj, PyObject *args, PyObject *kwargs, int mode);
PyObject *python_dir_sub_core(PyObject *obj, double arg, int mode);

// Math helper functions
PyObject *python_math_sub1(PyObject *self, PyObject *args, PyObject *kwargs, int mode);
PyObject *python_math_sub2(PyObject *self, PyObject *args, PyObject *kwargs, int mode);

// Sphere creation helpers
int sphereCalcIndInt(PyObject *func, Vector3d& dir);
int sphereCalcInd(/* PolySetBuilder &builder, */ std::vector<Vector3d>& vertices, PyObject *func,
                  Vector3d dir);
int sphereCalcSplitInd(/* PolySetBuilder &builder, */ std::vector<Vector3d>& vertices,
                       std::unordered_map<SphereEdgeDb, int /* , boost::hash<SphereEdgeDb> */>& edges,
                       PyObject *func, int ind1, int ind2);
std::unique_ptr<const Geometry> sphereCreateFuncGeometry(void *funcptr, double fs, int n);
std::unique_ptr<const Geometry> sheetCreateFuncGeometry(void *funcptr, double imin, double imax, double jmin, bool ispan, bool jspan);

// Debug modifier functions
PyObject *python_debug_modifier_func(PyObject *self, PyObject *args, PyObject *kwargs, int mode);
PyObject *python_debug_modifier_func_oo(PyObject *obj, PyObject *args, PyObject *kwargs, int mode);

// Utility functions for OpenSCAD integration
PyObject *python_osuse_include(int mode, PyObject *self, PyObject *args, PyObject *kwargs);
void python_str_sub(std::ostringstream& stream, const std::shared_ptr<AbstractNode>& node, int ident);
void python_export_obj_att(std::ostream& output);

// Python method tables
extern PyMethodDef PyOpenSCADFunctions[];
extern PyMethodDef PyOpenSCADMethods[];
extern PyNumberMethods PyOpenSCADNumbers;
extern PyMappingMethods PyOpenSCADMapping;

// Hash function for SphereEdgeDb
unsigned int hash_value(const SphereEdgeDb& r);
int operator==(const SphereEdgeDb& t1, const SphereEdgeDb& t2);

#endif  // PYFUNCTIONS_H

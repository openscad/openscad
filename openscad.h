/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef OPENSCAD_H
#define OPENSCAD_H

#ifdef ENABLE_OPENCSG
// this must be included before the GL headers
#  include <GL/glew.h>
#endif

#include <qgl.h>

#include <QHash>
#include <QCache>
#include <QVector>
#include <QProgressDialog>
#include <QSyntaxHighlighter>
#include <QPointer>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <fstream>
#include <iostream>

// for win32 and maybe others..
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

class Value;
class Expression;

class AbstractFunction;
class BuiltinFunction;
class Function;

class AbstractModule;
class ModuleInstantiation;
class Module;

class Context;
class PolySet;
class CSGTerm;
class CSGChain;
class AbstractNode;
class AbstractIntersectionNode;
class AbstractPolyNode;
struct CGAL_Nef_polyhedron;

template <typename T>
class Grid2d
{
public:
	double res;
	QHash<QPair<int,int>, T> db;

	Grid2d(double resolution = 0.001) {
		res = resolution;
	}
	T &align(double &x, double &y) {
		int ix = (int)round(x / res);
		int iy = (int)round(y / res);
		x = ix * res, y = iy * res;
		if (db.contains(QPair<int,int>(ix, iy)))
			return db[QPair<int,int>(ix, iy)];
		int dist = 10;
		T *ptr = NULL;
		for (int jx = ix - 1; jx <= ix + 1; jx++)
		for (int jy = iy - 1; jy <= iy + 1; jy++) {
			if (!db.contains(QPair<int,int>(jx, jy)))
				continue;
			if (abs(ix-jx) + abs(iy-jy) < dist) {
				x = jx * res, y = jy * res;
				dist = abs(ix-jx) + abs(iy-jy);
				ptr = &db[QPair<int,int>(jx, jy)];
			}
		}
		if (ptr)
			return *ptr;
		return db[QPair<int,int>(ix, iy)];
	}
	bool has(double x, double y) {
		int ix = (int)round(x / res);
		int iy = (int)round(y / res);
		if (db.contains(QPair<int,int>(ix, iy)))
			return true;
		for (int jx = ix - 1; jx <= ix + 1; jx++)
		for (int jy = iy - 1; jy <= iy + 1; jy++) {
			if (db.contains(QPair<int,int>(jx, jy)))
				return true;
		}
		return false;
	}
	bool eq(double x1, double y1, double x2, double y2) {
		align(x1, y1);
		align(x2, y2);
		if (fabs(x1 - x2) < res && fabs(y1 - y2) < res)
			return true;
		return false;
	}
	T &data(double x, double y) {
		return align(x, y);
	}
};

template <typename T>
class Grid3d
{
public:
	double res;
	QHash<QPair<QPair<int,int>,int>, T> db;

	Grid3d(double resolution = 0.001) {
		res = resolution;
	}
	T &align(double &x, double &y, double &z) {
		int ix = (int)round(x / res);
		int iy = (int)round(y / res);
		int iz = (int)round(z / res);
		x = ix * res, y = iy * res, z = iz * res;
		if (db.contains(QPair<QPair<int,int>,int>(QPair<int,int>(ix, iy), iz)))
			return db[QPair<QPair<int,int>,int>(QPair<int,int>(ix, iy), iz)];
		int dist = 10;
		T *ptr = NULL;
		for (int jx = ix - 1; jx <= ix + 1; jx++)
		for (int jy = iy - 1; jy <= iy + 1; jy++)
		for (int jz = iz - 1; jz <= iz + 1; jz++) {
			if (!db.contains(QPair<QPair<int,int>,int>(QPair<int,int>(jx, jy), jz)))
				continue;
			if (abs(ix-jx) + abs(iy-jy) + abs(iz-jz) < dist) {
				x = jx * res, y = jy * res, z = jz * res;
				dist = abs(ix-jx) + abs(iy-jy) + abs(iz-jz);
				ptr = &db[QPair<QPair<int,int>,int>(QPair<int,int>(jx, jy), jz)];
			}
		}
		if (ptr)
			return *ptr;
		return db[QPair<QPair<int,int>,int>(QPair<int,int>(ix, iy), iz)];
		
	}
	bool has(double x, double y, double z) {
		int ix = (int)round(x / res);
		int iy = (int)round(y / res);
		int iz = (int)round(z / res);
		if (db.contains(QPair<QPair<int,int>,int>(QPair<int,int>(ix, iy), iz)))
			return true;
		for (int jx = ix - 1; jx <= ix + 1; jx++)
		for (int jy = iy - 1; jy <= iy + 1; jy++)
		for (int jz = iz - 1; jz <= iz + 1; jz++) {
			if (db.contains(QPair<QPair<int,int>,int>(QPair<int,int>(jx, jy), jz)))
				return true;
		}
		return false;
		
	}
	bool eq(double x1, double y1, double z1, double x2, double y2, double z2) {
		align(x1, y1, z1);
		align(x2, y2, z2);
		if (fabs(x1 - x2) < res && fabs(y1 - y2) < res && fabs(z1 - z2) < res)
			return true;
		return false;
	}
	T &data(double x, double y, double z) {
		return align(x, y, z);
	}
};

class Value
{
public:
	enum type_e {
		UNDEFINED,
		BOOL,
		NUMBER,
		RANGE,
		VECTOR,
		STRING
	};

	enum type_e type;

	bool b;
	double num;
	QVector<Value*> vec;
	double range_begin;
	double range_step;
	double range_end;
	QString text;

	Value();
	~Value();

	Value(bool v);
	Value(double v);
	Value(const QString &t);

	Value(const Value &v);
	Value& operator = (const Value &v);

	Value operator ! () const;
	Value operator && (const Value &v) const;
	Value operator || (const Value &v) const;

	Value operator + (const Value &v) const;
	Value operator - (const Value &v) const;
	Value operator * (const Value &v) const;
	Value operator / (const Value &v) const;
	Value operator % (const Value &v) const;

	Value operator < (const Value &v) const;
	Value operator <= (const Value &v) const;
	Value operator == (const Value &v) const;
	Value operator != (const Value &v) const;
	Value operator >= (const Value &v) const;
	Value operator > (const Value &v) const;

	Value inv() const;

	bool getnum(double &v) const;
	bool getv2(double &x, double &y) const;
	bool getv3(double &x, double &y, double &z) const;

	QString dump() const;

private:
	void reset_undef();
};

class Expression
{
public:
	QVector<Expression*> children;

	Value *const_value;
	QString var_name;

	QString call_funcname;
	QVector<QString> call_argnames;

	// Boolean: ! && ||
	// Operators: * / % + -
	// Relations: < <= == != >= >
	// Vector element: []
	// Condition operator: ?:
	// Invert (prefix '-'): I
	// Constant value: C
	// Create Range: R
	// Create Vector: V
	// Create Matrix: M
	// Lookup Variable: L
	// Lookup member per name: N
	// Function call: F
	QString type;

	Expression();
	~Expression();

	Value evaluate(const Context *context) const;
	QString dump() const;
};

class AbstractFunction
{
public:
	virtual ~AbstractFunction();
	virtual Value evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const;
	virtual QString dump(QString indent, QString name) const;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef Value (*eval_func_t)(const QVector<QString> &argnames, const QVector<Value> &args);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual Value evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const;
	virtual QString dump(QString indent, QString name) const;
};

class Function : public AbstractFunction
{
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	Expression *expr;

	Function() { }
	virtual ~Function();

	virtual Value evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues) const;
	virtual QString dump(QString indent, QString name) const;
};

extern QHash<QString, AbstractFunction*> builtin_functions;
extern void initialize_builtin_functions();
extern void initialize_builtin_dxf_dim();
extern void destroy_builtin_functions();

class AbstractModule
{
public:
	virtual ~AbstractModule();
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
	virtual QString dump(QString indent, QString name) const;
};

class ModuleInstantiation
{
public:
	QString label;
	QString modname;
	QVector<QString> argnames;
	QVector<Expression*> argexpr;
	QVector<Value> argvalues;
	QVector<ModuleInstantiation*> children;

	bool tag_root;
	bool tag_highlight;
	bool tag_background;
	const Context *ctx;

	ModuleInstantiation() : tag_root(false), tag_highlight(false), tag_background(false), ctx(NULL) { }
	~ModuleInstantiation();

	QString dump(QString indent) const;
	AbstractNode *evaluate(const Context *ctx) const;
};

class Module : public AbstractModule
{
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	QVector<QString> assignments_var;
	QVector<Expression*> assignments_expr;

	QHash<QString, AbstractFunction*> functions;
	QHash<QString, AbstractModule*> modules;

	QVector<ModuleInstantiation*> children;

	Module() { }
	virtual ~Module();

	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
	virtual QString dump(QString indent, QString name) const;
};

extern QHash<QString, AbstractModule*> builtin_modules;
extern void initialize_builtin_modules();
extern void destroy_builtin_modules();

extern void register_builtin_csgops();
extern void register_builtin_transform();
extern void register_builtin_primitives();
extern void register_builtin_surface();
extern void register_builtin_control();
extern void register_builtin_render();
extern void register_builtin_import();
extern void register_builtin_dxf_linear_extrude();
extern void register_builtin_dxf_rotate_extrude();

class Context
{
public:
	const Context *parent;
	QHash<QString, Value> variables;
	QHash<QString, Value> config_variables;
	const QHash<QString, AbstractFunction*> *functions_p;
	const QHash<QString, AbstractModule*> *modules_p;

	static QVector<const Context*> ctx_stack;

	Context(const Context *parent = NULL);
	~Context();

	void args(const QVector<QString> &argnames, const QVector<Expression*> &argexpr, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues);

	void set_variable(QString name, Value value);
	Value lookup_variable(QString name, bool silent = false) const;

	Value evaluate_function(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues) const;
	AbstractNode *evaluate_module(const ModuleInstantiation *inst) const;
};

class DxfData
{
public:
	struct Point {
		double x, y;
		Point() : x(0), y(0) { }
		Point(double x, double y) : x(x), y(y) { }
	};
	struct Path {
		QList<Point*> points;
		bool is_closed, is_inner;
		Path() : is_closed(false), is_inner(false) { }
	};
	struct Dim {
		unsigned int type;
		double coords[7][2];
		double angle;
		QString name;
		Dim() {
			for (int i = 0; i < 7; i++)
			for (int j = 0; j < 2; j++)
				coords[i][j] = 0;
			type = 0;
		}
	};

	QList<Point> points;
	QList<Path> paths;
	QList<Dim> dims;

	DxfData();
	DxfData(double fn, double fs, double fa, QString filename, QString layername = QString(), double xorigin = 0.0, double yorigin = 0.0, double scale = 1.0);
	DxfData(const struct CGAL_Nef_polyhedron &N);

	Point *p(double x, double y);

private:
	void fixup_path_direction();
};

// The CGAL template magic slows down the compilation process by a factor of 5.
// So we only include the declaration of AbstractNode where it is needed...
#ifdef INCLUDE_ABSTRACT_NODE_DETAILS

#ifdef ENABLE_CGAL

#include <CGAL/Gmpq.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>

typedef CGAL::Extended_cartesian<CGAL::Gmpq> CGAL_Kernel2;
typedef CGAL::Nef_polyhedron_2<CGAL_Kernel2> CGAL_Nef_polyhedron2;
typedef CGAL_Kernel2::Aff_transformation_2 CGAL_Aff_transformation2;

typedef CGAL::Cartesian<CGAL::Gmpq> CGAL_Kernel3;
typedef CGAL::Polyhedron_3<CGAL_Kernel3> CGAL_Polyhedron;
typedef CGAL_Polyhedron::HalfedgeDS CGAL_HDS;
typedef CGAL::Polyhedron_incremental_builder_3<CGAL_HDS> CGAL_Polybuilder;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3> CGAL_Nef_polyhedron3;
typedef CGAL_Nef_polyhedron3::Aff_transformation_3 CGAL_Aff_transformation;
typedef CGAL_Nef_polyhedron3::Vector_3 CGAL_Vector;
typedef CGAL_Nef_polyhedron3::Plane_3 CGAL_Plane;
typedef CGAL_Nef_polyhedron3::Point_3 CGAL_Point;

struct CGAL_Nef_polyhedron
{
	int dim;
	CGAL_Nef_polyhedron2 p2;
	CGAL_Nef_polyhedron3 p3;

	CGAL_Nef_polyhedron() {
		dim = 0;
	}

	CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron2 &p) {
		dim = 2;
		p2 = p;
	}

	CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron3 &p) {
		dim = 3;
		p3 = p;
	}

	int weight() {
		if (dim == 2)
			return p2.explorer().number_of_vertices();
		if (dim == 3)
			return p3.number_of_vertices();
		return 0;
	}
};

#endif /* ENABLE_CGAL */

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

class PolySet
{
public:
	struct Point {
		double x, y, z;
		Point() : x(0), y(0), z(0) { }
		Point(double x, double y, double z) : x(x), y(y), z(z) { }
	};
	typedef QList<Point> Polygon;
	QVector<Polygon> polygons;
	QVector<Polygon> borders;
	Grid3d<void*> grid;

	bool is2d;
	int convexity;

	PolySet();
	~PolySet();

	void append_poly();
	void append_vertex(double x, double y, double z);
	void insert_vertex(double x, double y, double z);

	void append_vertex(double x, double y) {
		append_vertex(x, y, 0.0);
	}
	void insert_vertex(double x, double y) {
		insert_vertex(x, y, 0.0);
	}

	enum colormode_e {
		COLORMODE_NONE,
		COLORMODE_MATERIAL,
		COLORMODE_CUTOUT,
		COLORMODE_HIGHLIGHT,
		COLORMODE_BACKGROUND
	};

	enum csgmode_e {
		CSGMODE_NONE,
		CSGMODE_NORMAL = 1,
		CSGMODE_DIFFERENCE = 2,
		CSGMODE_BACKGROUND = 11,
		CSGMODE_BACKGROUND_DIFFERENCE = 12,
		CSGMODE_HIGHLIGHT = 21,
		CSGMODE_HIGHLIGHT_DIFFERENCE = 22
	};

	struct ps_cache_entry {
		PolySet *ps;
		QString msg;
		ps_cache_entry(PolySet *ps);
		~ps_cache_entry();
	};

	static QCache<QString,ps_cache_entry> ps_cache;

	void render_surface(colormode_e colormode, csgmode_e csgmode, double *m, GLint *shaderinfo = NULL) const;
	void render_edges(colormode_e colormode, csgmode_e csgmode) const;

#ifdef ENABLE_CGAL
	CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif

	int refcount;
	PolySet *link();
	void unlink();
};

class CSGTerm
{
public:
	enum type_e {
		TYPE_PRIMITIVE,
		TYPE_UNION,
		TYPE_INTERSECTION,
		TYPE_DIFFERENCE
	};

	type_e type;
	PolySet *polyset;
	QString label;
	CSGTerm *left;
	CSGTerm *right;
	double m[20];
	int refcounter;

	CSGTerm(PolySet *polyset, double m[20], QString label);
	CSGTerm(type_e type, CSGTerm *left, CSGTerm *right);

	CSGTerm *normalize();
	CSGTerm *normalize_tail();

	CSGTerm *link();
	void unlink();
	QString dump();
};

class CSGChain
{
public:
	QVector<PolySet*> polysets;
	QVector<double*> matrices;
	QVector<CSGTerm::type_e> types;
	QVector<QString> labels;

	CSGChain();

	void add(PolySet *polyset, double *m, CSGTerm::type_e type, QString label);
	void import(CSGTerm *term, CSGTerm::type_e type = CSGTerm::TYPE_UNION);
	QString dump();
};

class AbstractNode
{
public:
	QVector<AbstractNode*> children;
	const ModuleInstantiation *modinst;

	int progress_mark;
	void progress_prepare();
	void progress_report() const;

	int idx;
	static int idx_counter;
	QString dump_cache;

	AbstractNode(const ModuleInstantiation *mi);
	virtual ~AbstractNode();
	virtual QString mk_cache_id() const;
#ifdef ENABLE_CGAL
	struct cgal_nef_cache_entry {
		CGAL_Nef_polyhedron N;
		QString msg;
		cgal_nef_cache_entry(CGAL_Nef_polyhedron N);
	};
	static QCache<QString, cgal_nef_cache_entry> cgal_nef_cache;
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

class AbstractIntersectionNode : public AbstractNode
{
public:
	AbstractIntersectionNode(const ModuleInstantiation *mi) : AbstractNode(mi) { };
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

class AbstractPolyNode : public AbstractNode
{
public:
	enum render_mode_e {
		RENDER_CGAL,
		RENDER_OPENCSG
	};
	AbstractPolyNode(const ModuleInstantiation *mi) : AbstractNode(mi) { };
	virtual PolySet *render_polyset(render_mode_e mode) const;
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	static CSGTerm *render_csg_term_from_ps(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background, PolySet *ps, const ModuleInstantiation *modinst, int idx);
};

extern QHash<QString,Value> dxf_dim_cache;
extern QHash<QString,Value> dxf_cross_cache;

extern int progress_report_count;
extern void (*progress_report_f)(const class AbstractNode*, void*, int);
extern void *progress_report_vp;

void progress_report_prep(AbstractNode *root, void (*f)(const class AbstractNode *node, void *vp, int mark), void *vp);
void progress_report_fin();

void dxf_tesselate(PolySet *ps, DxfData *dxf, double rot, bool up, bool do_triangle_splitting, double h);
void dxf_border_to_ps(PolySet *ps, DxfData *dxf);

#endif /* INCLUDE_ABSTRACT_NODE_DETAILS */

class Highlighter : public QSyntaxHighlighter
{
public:
	Highlighter(QTextDocument *parent);
	void highlightBlock(const QString &text);
};

extern AbstractModule *parse(const char *text, int debug);
extern int get_fragments_from_r(double r, double fn, double fs, double fa);

extern QString commandline_commands;
extern int parser_error_pos;

#ifdef ENABLE_CGAL
void export_stl(CGAL_Nef_polyhedron *root_N, QString filename, QProgressDialog *pd);
void export_off(CGAL_Nef_polyhedron *root_N, QString filename, QProgressDialog *pd);
void export_dxf(CGAL_Nef_polyhedron *root_N, QString filename, QProgressDialog *pd);
#endif
extern void handle_dep(QString filename);

#endif


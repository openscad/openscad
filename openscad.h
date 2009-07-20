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

// this must be defined as early as possible
// so QHash<> and friends can see it..
#include <qglobal.h>
static inline uint qHash(double v) {
	// not beauty but good enough..
	union { double d; uint u[2]; } x;
	x.u[0] = 0; x.u[1] = 0; x.d = v;
	return x.u[0] ^ x.u[1];
}

#ifdef ENABLE_OPENCSG
// this must be included before the GL headers
#  include <GL/glew.h>
#endif

#include <QHash>
#include <QCache>
#include <QVector>
#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QGLWidget>
#include <QPointer>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fstream>
#include <iostream>

class Value;
class Expression;

class AbstractFunction;
class BuiltinFunction;
class Function;

class AbstractModule;
class ModuleInstanciation;
class Module;

class Context;
class PolySet;
class CSGTerm;
class CSGChain;
class AbstractNode;
class AbstractPolyNode;

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
	typedef Value (*eval_func_t)(const QVector<Value> &args);
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
extern void destroy_builtin_functions();

class AbstractModule
{
public:
	virtual ~AbstractModule();
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstanciation *inst) const;
	virtual QString dump(QString indent, QString name) const;
};

class ModuleInstanciation
{
public:
	QString label;
	QString modname;
	QVector<QString> argnames;
	QVector<Expression*> argexpr;
	QVector<Value> argvalues;
	QVector<ModuleInstanciation*> children;

	bool tag_root;
	bool tag_highlight;
	const Context *ctx;

	ModuleInstanciation() : tag_root(false), tag_highlight(false), ctx(NULL) { }
	~ModuleInstanciation();

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

	QVector<ModuleInstanciation*> children;

	Module() { }
	virtual ~Module();

	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstanciation *inst) const;
	virtual QString dump(QString indent, QString name) const;
};

extern QHash<QString, AbstractModule*> builtin_modules;
extern void initialize_builtin_modules();
extern void destroy_builtin_modules();

extern void register_builtin_csgops();
extern void register_builtin_transform();
extern void register_builtin_primitives();
extern void register_builtin_control();
extern void register_builtin_render();
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
	AbstractNode *evaluate_module(const ModuleInstanciation *inst) const;
};

class DxfData
{
public:
	struct Point {
		double x, y;
		Point() : x(0), y(0) { }
		Point(double x, double y) : x(x), y(y) { }
	};
	struct Line {
		Point *p[2];
		Line(Point *p1, Point *p2) { p[0] = p1; p[1] = p2; }
		Line() { p[0] = NULL; p[1] = NULL; }
	};
	struct Path {
		QList<Point*> points;
		bool is_closed, is_inner;
		Path() : is_closed(false), is_inner(false) { }
	};

	QList<Point> points;
	QList<Path> paths;

	DxfData(double fn, double fs, double fa, QString filename, QString layername = QString(), double xorigin = 0.0, double yorigin = 0.0, double scale = 1.0);

	Point *p(double x, double y);
};

// The CGAL template magic slows down the compilation process by a factor of 5.
// So we only include the declaration of AbstractNode where it is needed...
#ifdef INCLUDE_ABSTRACT_NODE_DETAILS

#ifdef ENABLE_CGAL

#include <CGAL/Gmpq.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>

typedef CGAL::Cartesian<CGAL::Gmpq> CGAL_Kernel;
typedef CGAL::Polyhedron_3<CGAL_Kernel> CGAL_Polyhedron;
typedef CGAL_Polyhedron::HalfedgeDS CGAL_HDS;
typedef CGAL::Polyhedron_incremental_builder_3<CGAL_HDS> CGAL_Polybuilder;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel> CGAL_Nef_polyhedron;
typedef CGAL_Nef_polyhedron::Aff_transformation_3  CGAL_Aff_transformation;
typedef CGAL_Nef_polyhedron::Vector_3 CGAL_Vector;
typedef CGAL_Nef_polyhedron::Plane_3 CGAL_Plane;
typedef CGAL_Nef_polyhedron::Point_3 CGAL_Point;

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
	double m[16];
	int convexity;

	PolySet();

	void append_poly();
	void append_vertex(double x, double y, double z);
	void insert_vertex(double x, double y, double z);

	void setmatrix(double m[16]);

	enum colormode_e {
		COLOR_NONE,
		COLOR_MATERIAL,
		COLOR_CUTOUT,
		COLOR_HIGHLIGHT
	};

	void render_surface(colormode_e colormode, GLint *shaderinfo = NULL) const;
	void render_edges(colormode_e colormode) const;

#ifdef ENABLE_CGAL
	CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
};

class CSGTerm
{
public:
	enum type_e {
		PRIMITIVE,
		UNION,
		INTERSECTION,
		DIFFERENCE
	};

	type_e type;
	PolySet *polyset;
	QString label;
	CSGTerm *left;
	CSGTerm *right;
	int refcounter;

	CSGTerm(PolySet *polyset, QString label);
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
	QVector<CSGTerm::type_e> types;
	QVector<QString> labels;

	CSGChain();

	void add(PolySet *polyset, CSGTerm::type_e type, QString label);
	void import(CSGTerm *term, CSGTerm::type_e type = CSGTerm::UNION);
	QString dump();
};

class AbstractNode
{
public:
	QVector<AbstractNode*> children;
	const ModuleInstanciation *modinst;

	int progress_mark;
	void progress_prepare();
	void progress_report() const;

	int idx;
	static int idx_counter;
	QString dump_cache;

	AbstractNode(const ModuleInstanciation *mi);
	virtual ~AbstractNode();
#ifdef ENABLE_CGAL
	static QCache<QString, CGAL_Nef_polyhedron> cgal_nef_cache;
	virtual QString cgal_nef_cache_id() const;
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[16], QVector<CSGTerm*> *highlights) const;
	virtual QString dump(QString indent) const;
};

class AbstractPolyNode : public AbstractNode
{
public:
	enum render_mode_e {
		RENDER_CGAL,
		RENDER_OPENCSG
	};
	AbstractPolyNode(const ModuleInstanciation *mi) : AbstractNode(mi) { };
	virtual PolySet *render_polyset(render_mode_e mode) const;
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
#endif
	virtual CSGTerm *render_csg_term(double m[16], QVector<CSGTerm*> *highlights) const;
};

extern int progress_report_count;
extern void (*progress_report_f)(const class AbstractNode*, void*, int);
extern void *progress_report_vp;

void progress_report_prep(AbstractNode *root, void (*f)(const class AbstractNode *node, void *vp, int mark), void *vp);
void progress_report_fin();

#else

// Needed for Mainwin::root_N
// this is a bit hackish - but a pointer is a pointer..
struct CGAL_Nef_polyhedron;

#endif /* HIDE_ABSTRACT_NODE_DETAILS */

class GLView : public QGLWidget
{
	Q_OBJECT

public:
	void (*renderfunc)(void*);
	void *renderfunc_vp;

	double viewer_distance;
	double object_rot_y;
	double object_rot_z;

	double w_h_ratio;
	GLint shaderinfo[11];

	GLView(QWidget *parent = NULL);

protected:
	bool mouse_drag_active;
	int last_mouse_x;
	int last_mouse_y;

	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	QString filename;
	QSplitter *s1, *s2;
	QTextEdit *editor;
	GLView *screen;
	QTextEdit *console;

	Context root_ctx;
	AbstractModule *root_module;
	AbstractNode *absolute_root_node;
	CSGTerm *root_raw_term;
	CSGTerm *root_norm_term;
	CSGChain *root_chain;
#ifdef ENABLE_CGAL
	CGAL_Nef_polyhedron *root_N;
#endif

	QVector<CSGTerm*> highlight_terms;
	CSGChain *highlights_chain;
	AbstractNode *root_node;
	bool enableOpenCSG;

	MainWindow(const char *filename = 0);
	~MainWindow();

private:
	void load();
	void find_root_tag(AbstractNode *n);
	void compile();

private slots:
	void actionNew();
	void actionOpen();
	void actionSave();
	void actionSaveAs();
	void actionReload();

private slots:
	void editIndent();
	void editUnindent();
	void editComment();
	void editUncomment();

private slots:
	void actionReloadCompile();
	void actionCompile();
#ifdef ENABLE_CGAL
	void actionRenderCGAL();
#endif
	void actionDisplayAST();
	void actionDisplayCSGTree();
	void actionDisplayCSGProducts();
	void actionExportSTL();
	void actionExportOFF();

public:
#ifdef ENABLE_OPENCSG
	QAction *actViewModeOpenCSG;
#endif
#ifdef ENABLE_CGAL
	QAction *actViewModeCGALSurface;
	QAction *actViewModeCGALGrid;
#endif
	QAction *actViewModeThrownTogether;
	QAction *actViewModeShowEdges;
	void viewModeActionsUncheck();

private slots:
#ifdef ENABLE_OPENCSG
	void viewModeOpenCSG();
#endif
#ifdef ENABLE_CGAL
	void viewModeCGALSurface();
	void viewModeCGALGrid();
#endif
	void viewModeThrownTogether();
	void viewModeShowEdges();
};

extern AbstractModule *parse(const char *text, int debug);
extern int get_fragments_from_r(double r, double fn, double fs, double fa);

extern QPointer<MainWindow> current_win;

#define PRINT(_msg) do { if (current_win.isNull()) fprintf(stderr, "%s\n", QString(_msg).toAscii().data()); else current_win->console->append(_msg); } while (0)
#define PRINTF(_fmt, ...) do { QString _m; _m.sprintf(_fmt, ##__VA_ARGS__); PRINT(_m); } while (0)
#define PRINTA(_fmt, ...) do { QString _m = QString(_fmt).arg(__VA_ARGS__); PRINT(_m); } while (0)

#endif


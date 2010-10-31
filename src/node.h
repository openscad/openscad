#ifndef NODE_H_
#define NODE_H_

#include <QCache>
#include <QVector>

#ifdef ENABLE_CGAL
#include "cgal.h"
#endif

#include "traverser.h"

extern int progress_report_count;
extern void (*progress_report_f)(const class AbstractNode*, void*, int);
extern void *progress_report_vp;

void progress_report_prep(AbstractNode *root, void (*f)(const class AbstractNode *node, void *vp, int mark), void *vp);
void progress_report_fin();

class AbstractNode
{
	static int idx_counter;   // Node instantiation index
public:
	AbstractNode(const class ModuleInstantiation *mi);
	virtual ~AbstractNode();
  virtual Response accept(const class State &state, class Visitor &visitor) const;
	virtual std::string toString() const;

  // FIXME: Make return value a reference
	const std::list<AbstractNode*> getChildren() const { 
		return this->children.toList().toStdList(); 
	}
	int index() const { return this->idx; }

	static void resetIndexCounter() { idx_counter = 1; }

	QVector<AbstractNode*> children;
	const ModuleInstantiation *modinst;
	
	int progress_mark;
	void progress_prepare();
	void progress_report() const;

	int idx; // Node index (unique per tree)
	QString dump_cache;



	virtual QString mk_cache_id() const;
#ifdef ENABLE_CGAL
	struct cgal_nef_cache_entry {
		CGAL_Nef_polyhedron N;
		QString msg;
		cgal_nef_cache_entry(const CGAL_Nef_polyhedron &N);
	};
	static QCache<QString, cgal_nef_cache_entry> cgal_nef_cache;
	virtual CGAL_Nef_polyhedron renderCSGMesh() const;
	class CSGTerm *render_csg_term_from_nef(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background, const char *statement, int convexity) const;
#endif
	virtual class CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

class AbstractIntersectionNode : public AbstractNode
{
public:
	AbstractIntersectionNode(const ModuleInstantiation *mi) : AbstractNode(mi) { };
	virtual ~AbstractIntersectionNode() { };
  virtual Response accept(const class State &state, class Visitor &visitor) const;
	virtual std::string toString() const;

#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron renderCSGMesh() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

class AbstractPolyNode : public AbstractNode
{
public:
	AbstractPolyNode(const ModuleInstantiation *mi) : AbstractNode(mi) { };
	virtual ~AbstractPolyNode() { };
  virtual Response accept(const class State &state, class Visitor &visitor) const;

	enum render_mode_e {
		RENDER_CGAL,
		RENDER_OPENCSG
	};
	virtual class PolySet *render_polyset(render_mode_e mode) const = 0;
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron renderCSGMesh() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	static CSGTerm *render_csg_term_from_ps(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background, PolySet *ps, const ModuleInstantiation *modinst, int idx);
};

std::ostream &operator<<(std::ostream &stream, const AbstractNode &node);
// FIXME: Doesn't belong here..
std::ostream &operator<<(std::ostream &stream, const QString &str);

#endif

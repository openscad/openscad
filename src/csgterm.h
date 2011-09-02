#ifndef CSGTERM_H_
#define CSGTERM_H_

#include <QString>
#include <QVector>
#include "polyset.h"

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
	double m[16];
	double color[4];
	int refcounter;

	CSGTerm(PolySet *polyset, const double matrix[16], const double color[4], QString label);
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
	QVector<double*> colors;
	QVector<CSGTerm::type_e> types;
	QVector<QString> labels;

	CSGChain();

	void add(PolySet *polyset, double *m, double *color, CSGTerm::type_e type, QString label);
	void import(CSGTerm *term, CSGTerm::type_e type = CSGTerm::TYPE_UNION);
	QString dump();

	BoundingBox getBoundingBox() const;
};

#endif

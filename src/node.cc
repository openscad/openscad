/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
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

#include "printutils.h"
#include "node.h"
#include "module.h"
#include "csgterm.h"
#include "progress.h"
#include "polyset.h"
#include "visitor.h"
#include "nodedumper.h"

#include <QRegExp>
#include <sstream>

int AbstractNode::idx_counter;

AbstractNode::AbstractNode(const ModuleInstantiation *mi)
{
	modinst = mi;
	idx = idx_counter++;
}

AbstractNode::~AbstractNode()
{
	foreach (AbstractNode *v, children)
		delete v;
}

Response AbstractNode::accept(const class State &state, Visitor &visitor) const
{
	return visitor.visit(state, *this);
}

Response AbstractIntersectionNode::accept(const class State &state, Visitor &visitor) const
{
	return visitor.visit(state, *this);
}

Response AbstractPolyNode::accept(const class State &state, Visitor &visitor) const
{
	return visitor.visit(state, *this);
}

std::string AbstractNode::toString() const
{
	std::stringstream stream;
	stream << "group()";
	return stream.str();
}

std::string AbstractIntersectionNode::toString() const
{
	std::stringstream stream;
	stream << "intersection()";
	return stream.str();
}

void AbstractNode::progress_prepare()
{
	foreach (AbstractNode *v, children)
		v->progress_prepare();
	this->progress_mark = ++progress_report_count;
}

void AbstractNode::progress_report() const
{
	progress_update(this, this->progress_mark);
}

static CSGTerm *render_csg_term_backend(const AbstractNode *that, bool intersect, double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background)
{
	CSGTerm *t1 = NULL;
	foreach(AbstractNode *v, that->children) {
		CSGTerm *t2 = v->render_csg_term(m, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (intersect)
				t1 = new CSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			else
				t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
		}
	}
	if (t1 && that->modinst->tag_highlight && highlights)
		highlights->append(t1->link());
	if (t1 && that->modinst->tag_background && background) {
		background->append(t1);
		return NULL;
	}
	return t1;
}

CSGTerm *AbstractNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	return render_csg_term_backend(this, false, m, highlights, background);
}

CSGTerm *AbstractIntersectionNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	return render_csg_term_backend(this, true, m, highlights, background);
}

CSGTerm *AbstractPolyNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	PolySet *ps = render_polyset(RENDER_OPENCSG);
	return render_csg_term_from_ps(m, highlights, background, ps, modinst, idx);
}

CSGTerm *AbstractPolyNode::render_csg_term_from_ps(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background, PolySet *ps, const ModuleInstantiation *modinst, int idx)
{
	CSGTerm *t = new CSGTerm(ps, m, QString("n%1").arg(idx));
	if (modinst->tag_highlight && highlights)
		highlights->append(t->link());
	if (modinst->tag_background && background) {
		background->append(t);
		return NULL;
	}
	return t;
}

std::ostream &operator<<(std::ostream &stream, const QString &str)
{
	stream << str.toStdString();
	return stream;
}

std::ostream &operator<<(std::ostream &stream, const AbstractNode &node)
{
	stream << node.toString();
	return stream;
}

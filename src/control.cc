/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
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

#include <boost/foreach.hpp>
#include "module.h"
#include "node.h"
#include "evalcontext.h"
#include "modcontext.h"
#include "expression.h"
#include "builtin.h"
#include "printutils.h"
#include <sstream>
#include "mathc99.h"

// pour le probe()
#include "CGAL_Nef_polyhedron.h"
#include "GeometryEvaluator.h"
//#include "PolySetCGALEvaluator.h"
#include "Tree.h"
//#include "polyset.h"
//#include "CGALCache.h"
//#include "cgal.h"
#include "cgalutils.h"



#define foreach BOOST_FOREACH


class ControlModule : public AbstractModule
{
public: // types
	enum Type {
		CHILD,
		CHILDREN,
		ECHO,
		ASSIGN,
		FOR,
		INT_FOR,
		IF,
		PROBE
    };
public: // methods
	ControlModule(Type type)
		: type(type)
	{ }

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;

	static void for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l, 
						 const Context *ctx, const EvalContext *evalctx);

	static const EvalContext* getLastModuleCtx(const EvalContext *evalctx);
	
	static AbstractNode* getChild(const ValuePtr &value, const EvalContext* modulectx);

private: // data
	Type type;

}; // class ControlModule

void ControlModule::for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l, 
							const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() > l) {
		const std::string &it_name = evalctx->getArgName(l);
		ValuePtr it_values = evalctx->getArgValue(l, ctx);
		Context c(ctx);
		if (it_values->type() == Value::RANGE) {
			RangeType range = it_values->toRange();
			boost::uint32_t steps = range.numValues();
			if (steps >= 10000) {
				PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
			} else {
				for (RangeType::iterator it = range.begin();it != range.end();it++) {
					c.set_variable(it_name, ValuePtr(*it));
					for_eval(node, inst, l+1, &c, evalctx);
				}
			}
		}
		else if (it_values->type() == Value::VECTOR) {
			for (size_t i = 0; i < it_values->toVector().size(); i++) {
				c.set_variable(it_name, it_values->toVector()[i]);
				for_eval(node, inst, l+1, &c, evalctx);
			}
		}
		else if (it_values->type() != Value::UNDEFINED) {
			c.set_variable(it_name, it_values);
			for_eval(node, inst, l+1, &c, evalctx);
		}
	} else if (l > 0) {
		// At this point, the for loop variables have been set and we can initialize
		// the local scope (as they may depend on the for loop variables
		Context c(ctx);
		BOOST_FOREACH(const Assignment &ass, inst.scope.assignments) {
			c.set_variable(ass.first, ass.second->evaluate(&c));
		}
		
		std::vector<AbstractNode *> instantiatednodes = inst.instantiateChildren(&c);
		node.children.insert(node.children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
}

const EvalContext* ControlModule::getLastModuleCtx(const EvalContext *evalctx)
{
	// Find the last custom module invocation, which will contain
	// an eval context with the children of the module invokation
	const Context *tmpc = evalctx;
	while (tmpc->getParent()) {
		const ModuleContext *modulectx = dynamic_cast<const ModuleContext*>(tmpc->getParent());
		if (modulectx) {
			// This will trigger if trying to invoke child from the root of any file
			// assert(filectx->evalctx);
			if (modulectx->evalctx) {
				return modulectx->evalctx;
			}
			return NULL;
		}
		tmpc = tmpc->getParent();
	}
	return NULL;
}

// static
AbstractNode* ControlModule::getChild(const ValuePtr &value, const EvalContext* modulectx)
{
	if (value->type()!=Value::NUMBER) {
		// Invalid parameter
		// (e.g. first child of difference is invalid)
		PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value->toString());
		return NULL;
	}
	double v;
	if (!value->getDouble(v)) {
		PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value->toString());
		return NULL;
	}
		
	int n = trunc(v);
	if (n < 0) {
		PRINTB("WARNING: Negative children index (%d) not allowed", n);
		return NULL; // Disallow negative child indices
	}
	if (n>=(int)modulectx->numChildren()) {
		// How to deal with negative objects in this case?
		// (e.g. first child of difference is invalid)
		PRINTB("WARNING: Children index (%d) out of bounds (%d children)"
			, n % modulectx->numChildren());
		return NULL;
	}
	// OK
	return modulectx->getChild(n)->evaluate(modulectx);
}

AbstractNode *ControlModule::instantiate(const Context* /*ctx*/, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	AbstractNode *node = NULL;

	switch (this->type) {
	case CHILD:	{
		printDeprecation("child() will be removed in future releases. Use children() instead.");
		int n = 0;
		if (evalctx->numArgs() > 0) {
			double v;
			if (evalctx->getArgValue(0)->getDouble(v)) {
				n = trunc(v);
				if (n < 0) {
					PRINTB("WARNING: Negative child index (%d) not allowed", n);
					return NULL; // Disallow negative child indices
				}
			}
		}

		// Find the last custom module invocation, which will contain
		// an eval context with the children of the module invokation
		const EvalContext *modulectx = getLastModuleCtx(evalctx);
		if (modulectx==NULL) {
			return NULL;
		}
		// This will trigger if trying to invoke child from the root of any file
        if (n < (int)modulectx->numChildren()) {
			node = modulectx->getChild(n)->evaluate(modulectx);
		}
		else {
			// How to deal with negative objects in this case?
            // (e.g. first child of difference is invalid)
			PRINTB("WARNING: Child index (%d) out of bounds (%d children)", 
				   n % modulectx->numChildren());
		}
		return node;
	}
		break;

	case CHILDREN: {
		const EvalContext *modulectx = getLastModuleCtx(evalctx);
		if (modulectx==NULL) {
			return NULL;
		}
		// This will trigger if trying to invoke child from the root of any file
		// assert(filectx->evalctx);
		if (evalctx->numArgs()<=0) {
			// no parameters => all children
			AbstractNode* node = new GroupNode(inst);
			for (int n = 0; n < (int)modulectx->numChildren(); ++n) {
				AbstractNode* childnode = modulectx->getChild(n)->evaluate(modulectx);
				if (childnode==NULL) continue; // error
				node->children.push_back(childnode);
			}
			return node;
		}
		else if (evalctx->numArgs()>0) {
			// one (or more ignored) parameter
			ValuePtr value = evalctx->getArgValue(0);
			if (value->type() == Value::NUMBER) {
				return getChild(value, modulectx);
			}
			else if (value->type() == Value::VECTOR) {
				AbstractNode* node = new GroupNode(inst);
				const Value::VectorType& vect = value->toVector();
				foreach (const ValuePtr &vectvalue, vect) {
					AbstractNode* childnode = getChild(vectvalue,modulectx);
					if (childnode==NULL) continue; // error
					node->children.push_back(childnode);
				}
				return node;
			}
			else if (value->type() == Value::RANGE) {
				RangeType range = value->toRange();
				boost::uint32_t steps = range.numValues();
				if (steps >= 10000) {
					PRINTB("WARNING: Bad range parameter for children: too many elements (%lu).", steps);
					return NULL;
				}
				AbstractNode* node = new GroupNode(inst);
				for (RangeType::iterator it = range.begin();it != range.end();it++) {
					AbstractNode* childnode = getChild(ValuePtr(*it),modulectx); // with error cases
					if (childnode==NULL) continue; // error
					node->children.push_back(childnode);
				}
				return node;
			}
			else {
				// Invalid parameter
				// (e.g. first child of difference is invalid)
				PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value->toString());
				return NULL;
			}
		}
		return NULL;
	}
		break;

	case ECHO: {
		node = new GroupNode(inst);
		std::stringstream msg;
		msg << "ECHO: ";
		for (size_t i = 0; i < inst->arguments.size(); i++) {
			if (i > 0) msg << ", ";
			if (!evalctx->getArgName(i).empty()) msg << evalctx->getArgName(i) << " = ";
			ValuePtr val = evalctx->getArgValue(i);
			if (val->type() == Value::STRING) {
				msg << '"' << val->toString() << '"';
			} else {
				msg << val->toString();
			}
		}
		PRINTB("%s", msg.str());
	}
		break;

	case ASSIGN: {
		node = new GroupNode(inst);
		// We create a new context to avoid parameters from influencing each other
		// -> parallel evaluation. This is to be backwards compatible.
		Context c(evalctx);
		for (size_t i = 0; i < evalctx->numArgs(); i++) {
			if (!evalctx->getArgName(i).empty())
				c.set_variable(evalctx->getArgName(i), evalctx->getArgValue(i));
		}
		// Let any local variables override the parameters
		inst->scope.apply(c);
		std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(&c);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
		break;

	case FOR:
		node = new GroupNode(inst);
		for_eval(*node, *inst, 0, evalctx, evalctx);
		break;

	case INT_FOR:
		node = new AbstractIntersectionNode(inst);
		for_eval(*node, *inst, 0, evalctx, evalctx);
		break;

	case IF: {
		node = new GroupNode(inst);
		const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation*>(inst);
		if (evalctx->numArgs() > 0 && evalctx->getArgValue(0)->toBool()) {
			inst->scope.apply(*evalctx);
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		}
		else {
			ifelse->else_scope.apply(*evalctx);
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateElseChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		}
	}
		break;
	case PROBE: {
		//
		// render first children, then compute the bounding box.
		// set the following 4 vector variables and 1 bool variable:
		//    bbempty = true/false, state if there was any usable geometry.
		//              when bbempty is false, the next variables are undef
		//    bbmin = [xmin,ymin,zmin], the minimum of the bounding box
		//    bbmax = [xmax,ymax,zmax], the maximum of the bounding box
		//    bbsize = [xmax-xmin,...], the size of the bounding box
		//    bbcenter = [(xmax+xmin)/2, ...], the center of the bounding box
		//
		// the only parameter that probe takes is $exact=true/false
		// it is generaly set to true, but with false the rendering will not be Nef (so its faster).
		// any other parameter will be treated just like assign() (i.e. passed inside)
		//
		node = new AbstractNode(inst);
		Context c(evalctx);

		// les parametres.. au cas ou on fera $exact=1
		for (size_t i = 0; i < evalctx->numArgs(); i++) {
			if (!evalctx->getArgName(i).empty()) {
				c.set_variable(evalctx->getArgName(i), evalctx->getArgValue(i));
			}
		}

		// not sure how to set the default value to true... for now its false.
        	bool exact = c.lookup_variable("$exact")->toBool();

		// Let any local variables override the parameters
		inst->scope.apply(c);

		// instantiate children one by one...
                std::vector<AbstractNode*> childnodes;
                AbstractNode *nc;

		double xmin,ymin,zmin,xmax,ymax,zmax;

                for(unsigned int k=0;k<evalctx->numChildren();k++) {
                        nc = evalctx->getChild(k)->evaluate(&c);
                        // first child? then we render and set the bbox variables
                        if( k==0 && nc!=NULL ) {
                                Tree tree;
                                tree.setRoot(nc);
                                GeometryEvaluator geomEvaluator(tree);

				shared_ptr<const PolySet> G;
				shared_ptr<const CGAL_Nef_polyhedron> N;
				shared_ptr<const Geometry> geom;

				bool empty=true;

				geom=geomEvaluator.evaluateGeometry(*nc,exact); // false-> no NEF, true= ok NEF
				G = dynamic_pointer_cast<const PolySet>(geom);
				// we assueme that we will get either CSG or CGAL, but not both
				if( G!=NULL ) {
					// we obtained a fast CSG geometry instead of a Nef polyhedron.
					empty=G->isEmpty();
					if( !empty ) {
						BoundingBox bb = G->getBoundingBox();
						xmin=bb.min().x();
						ymin=bb.min().y();
						zmin=bb.min().z();
						xmax=bb.max().x();
						ymax=bb.max().y();
						zmax=bb.max().z();
					}
				}else{
#ifdef ENABLE_CGAL
					N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
					if( N!=NULL ) {
					    empty=N->isEmpty();
					    if( !empty ) {
						CGAL_Iso_cuboid_3 bb;
						bb = CGALUtils::boundingBox( *(N->p3) );
						xmin=CGAL::to_double(bb.xmin());
						ymin=CGAL::to_double(bb.ymin());
						zmin=CGAL::to_double(bb.zmin());
						xmax=CGAL::to_double(bb.xmax());
						ymax=CGAL::to_double(bb.ymax());
						zmax=CGAL::to_double(bb.zmax());
					    }
					}
#endif
				}
			        c.set_variable("bbempty",Value(empty));
				if( !empty ) {
					// define the variables
                                        Value::VectorType bbmin;
					bbmin.push_back(xmin);
					bbmin.push_back(ymin);
					bbmin.push_back(zmin);
                                        c.set_variable("bbmin",Value(bbmin));

                                        Value::VectorType bbmax;
					bbmax.push_back(xmax);
					bbmax.push_back(ymax);
					bbmax.push_back(zmax);
                                        c.set_variable("bbmax",Value(bbmax));

                                        Value::VectorType bbcenter;
                                        bbcenter.push_back((xmin+xmax)/2.0);
                                        bbcenter.push_back((ymin+ymax)/2.0);
                                        bbcenter.push_back((zmin+zmax)/2.0);
                                        c.set_variable("bbcenter",Value(bbcenter));

                                        Value::VectorType bbsize;
                                        bbsize.push_back(xmax-xmin);
                                        bbsize.push_back(ymax-ymin);
                                        bbsize.push_back(zmax-zmin);
                                        c.set_variable("bbsize",Value(bbsize));
				}
				// this node is not added to the final rendering.
				delete nc;
			}else{
				// add the node to the final rendering
				if( nc!=NULL ) node->children.push_back(nc);
			}
		}

		//std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(&c);
		//node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		break;
	}
	}
	return node;
}

void register_builtin_control()
{
	Builtins::init("child", new ControlModule(ControlModule::CHILD));
	Builtins::init("children", new ControlModule(ControlModule::CHILDREN));
	Builtins::init("echo", new ControlModule(ControlModule::ECHO));
	Builtins::init("assign", new ControlModule(ControlModule::ASSIGN));
	Builtins::init("for", new ControlModule(ControlModule::FOR));
	Builtins::init("intersection_for", new ControlModule(ControlModule::INT_FOR));
	Builtins::init("if", new ControlModule(ControlModule::IF));
	Builtins::init("probe", new ControlModule(ControlModule::PROBE));
}

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

#include "module.h"
#include "node.h"
#include "polyset.h"
#include "evalcontext.h"
#include "Polygon2d.h"
#include "builtin.h"
#include "printutils.h"
#include "context.h"
#include "calc.h"
#include "degree_trig.h"
#include <sstream>
#include <assert.h>
#include <cmath>
#include <boost/assign/std/vector.hpp>
#include "ModuleInstantiation.h"
#include "boost-utils.h"
using namespace boost::assign; // bring 'operator+=()' into scope

#define F_MINIMUM 0.01

enum class primitive_type_e {
	CUBE,
	SPHERE,
	CYLINDER,
	POLYHEDRON,
	SQUARE,
	CIRCLE,
	POLYGON
};

class PrimitiveModule : public AbstractModule
{
public:
	primitive_type_e type;
	PrimitiveModule(primitive_type_e type) : type(type) { }
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
private:
	Value lookup_radius(const std::shared_ptr<Context> ctx, const Location &loc, const std::string &radius_var, const std::string &diameter_var) const;
};

class PrimitiveNode : public LeafNode
{
public:
	VISITABLE();
	PrimitiveNode(const ModuleInstantiation *mi, const std::shared_ptr<EvalContext> &ctx, primitive_type_e type, const std::string &docPath) : LeafNode(mi, ctx),
			document_path(docPath), type(type), points(Value::undefined.clone()), paths(Value::undefined.clone()), faces(Value::undefined.clone()) { }
	std::string toString() const override;
	std::string name() const override {
		switch (this->type) {
		case primitive_type_e::CUBE:       return "cube";
		case primitive_type_e::SPHERE:     return "sphere";
		case primitive_type_e::CYLINDER:   return "cylinder";
		case primitive_type_e::POLYHEDRON: return "polyhedron";
		case primitive_type_e::SQUARE:     return "square";
		case primitive_type_e::CIRCLE:     return "circle";
		case primitive_type_e::POLYGON:    return "polygon";
		default: assert(false && "PrimitiveNode::name(): Unknown primitive type");
		}
		return "unknown";
	}
	const std::string document_path;

	bool center;
	double x, y, z, h, r1, r2;
	double fn, fs, fa;
	primitive_type_e type;
	int convexity;
	Value points, paths, faces;
	const Geometry *createGeometry() const override;
};

/**
 * Return a radius value by looking up both a diameter and radius variable.
 * The diameter has higher priority, so if found an additionally set radius
 * value is ignored.
 *
 * @param ctx data context with variable values.
 * @param radius_var name of the variable to lookup for the radius value.
 * @param diameter_var name of the variable to lookup for the diameter value.
 * @return radius value of type Value::Type::NUMBER or Value::Type::UNDEFINED if both
 *         variables are invalid or not set.
 */
Value PrimitiveModule::lookup_radius(const std::shared_ptr<Context> ctx, const Location &loc, const std::string &diameter_var, const std::string &radius_var) const
{
	const auto &d = ctx->lookup_variable(diameter_var, true);
	const auto &r = ctx->lookup_variable(radius_var, true);
	const auto r_defined = (r.type() == Value::Type::NUMBER);

	if (d.type() == Value::Type::NUMBER) {
		if (r_defined) {
			LOG(message_group::Warning,loc,ctx->documentPath(),
				"Ignoring radius variable '%1$s' as diameter '%2$s' is defined too.",radius_var,diameter_var);
		}
		return d.toDouble() / 2.0;
	} else if (r_defined) {
		return r.clone();
	} else {
		return Value::undefined.clone();
	}
}

AbstractNode *PrimitiveModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new PrimitiveNode(inst, evalctx, this->type, ctx->documentPath());

	node->center = false;
	node->x = node->y = node->z = node->h = node->r1 = node->r2 = 1;

	AssignmentList args;
	AssignmentList optargs;
	if(inst->scope.hasChildren()){
		LOG(message_group::Warning,inst->location(),ctx->documentPath(),
			"module %1$s() does not support child modules",node->name());
	}

	switch (this->type) {
	case primitive_type_e::CUBE:
		args += assignment("size"), assignment("center");
		break;
	case primitive_type_e::SPHERE:
		args += assignment("r");
		optargs += assignment("d");
		break;
	case primitive_type_e::CYLINDER:
		args += assignment("h"), assignment("r1"), assignment("r2"), assignment("center");
		optargs += assignment("r"),  assignment("d"), assignment("d1"),  assignment("d2");
		break;
	case primitive_type_e::POLYHEDRON:
		args += assignment("points"), assignment("faces"), assignment("convexity");
		optargs += assignment("triangles");
		break;
	case primitive_type_e::SQUARE:
		args += assignment("size"), assignment("center");
		break;
	case primitive_type_e::CIRCLE:
		args += assignment("r");
		optargs += assignment("d");
		break;
	case primitive_type_e::POLYGON:
		args += assignment("points"), assignment("paths"), assignment("convexity");
		break;
	default:
		assert(false && "PrimitiveModule::instantiate(): Unknown node type");
	}

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args, optargs);

	node->fn = c->lookup_variable("$fn").toDouble();
	node->fs = c->lookup_variable("$fs").toDouble();
	node->fa = c->lookup_variable("$fa").toDouble();

	if (node->fs < F_MINIMUM) {
		LOG(message_group::Warning,inst->location(),ctx->documentPath(),
			"$fs too small - clamping to %1$f",F_MINIMUM);
		node->fs = F_MINIMUM;
	}
	if (node->fa < F_MINIMUM) {
		LOG(message_group::Warning,inst->location(),ctx->documentPath(),
			"$fa too small - clamping to %1$f",F_MINIMUM);
		node->fa = F_MINIMUM;
	}

	switch (this->type)  {
	case primitive_type_e::CUBE: {
		const auto &size = c->lookup_variable("size");
		const auto &center = c->lookup_variable("center");
		if (size.isDefined()) {
			bool converted=false;
			converted |= size.getDouble(node->x);
			converted |= size.getDouble(node->y);
			converted |= size.getDouble(node->z);
			converted |= size.getVec3(node->x, node->y, node->z);
			if (!converted) {
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),"Unable to convert cube(size=%1$s, ...) parameter to a number or a vec3 of numbers",size.toEchoString());
			} else if(OpenSCAD::rangeCheck) {
				bool ok = (node->x > 0) && (node->y > 0) && (node->z > 0);
				ok &= std::isfinite(node->x) && std::isfinite(node->y) && std::isfinite(node->z);
				if(!ok){
					LOG(message_group::Warning,inst->location(),ctx->documentPath(),"cube(size=%1$s, ...)",size.toEchoString());
				}
			}
		}
		if (center.type() == Value::Type::BOOL) {
			node->center = center.toBool();
		}
		break;
	}
	case primitive_type_e::SPHERE: {
		const auto r = lookup_radius(c.ctx, inst->location(), "d", "r");
		if (r.type() == Value::Type::NUMBER) {
			node->r1 = r.toDouble();
			if (OpenSCAD::rangeCheck && (node->r1 <= 0 || !std::isfinite(node->r1))){
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),
					"sphere(r=%1$s)",r.toEchoString());
			}
		}
		break;
	}
	case primitive_type_e::CYLINDER: {
		const auto &h = c->lookup_variable("h");
		if (h.type() == Value::Type::NUMBER) {
			node->h = h.toDouble();
		}

		auto r = lookup_radius(c.ctx, inst->location(), "d", "r");
		auto r1 = lookup_radius(c.ctx, inst->location(), "d1", "r1");
		auto r2 = lookup_radius(c.ctx, inst->location(), "d2", "r2");
		if(r.type() == Value::Type::NUMBER &&
			(r1.type() == Value::Type::NUMBER || r2.type() == Value::Type::NUMBER)
			){
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),
					"Cylinder parameters ambiguous");
		}

		if (r.type() == Value::Type::NUMBER) {
			node->r1 = r.toDouble();
			node->r2 = r.toDouble();
		}
		if (r1.type() == Value::Type::NUMBER) {
			node->r1 = r1.toDouble();
		}
		if (r2.type() == Value::Type::NUMBER) {
			node->r2 = r2.toDouble();
		}

		if(OpenSCAD::rangeCheck){
			if (node->h <= 0 || !std::isfinite(node->h)){
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),"cylinder(h=%1$s, ...)",h.toEchoString());
			}
			if (node->r1 < 0 || node->r2 < 0 || (node->r1 == 0 && node->r2 == 0) || !std::isfinite(node->r1) || !std::isfinite(node->r2)){
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),
					"cylinder(r1=%1$s, r2=%2$s, ...)",
					(r1.type() == Value::Type::NUMBER ? r1.toEchoString() : r.toEchoString()),
					(r2.type() == Value::Type::NUMBER ? r2.toEchoString() : r.toEchoString()));
			}
		}

		const auto &center = c->lookup_variable("center");
		if (center.type() == Value::Type::BOOL) {
			node->center = center.toBool();
		}
		break;
	}
	case primitive_type_e::POLYHEDRON: {
		node->points = c->lookup_variable("points").clone();
		node->faces = c->lookup_variable("faces").clone();
		if (node->faces.type() == Value::Type::UNDEFINED) {
			// backwards compatible
			node->faces = c->lookup_variable("triangles", true).clone();
			if (node->faces.type() != Value::Type::UNDEFINED) {
				LOG(message_group::Deprecated,Location::NONE,"","polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
			}
		}
		break;
	}
	case primitive_type_e::SQUARE: {
		const auto &size = c->lookup_variable("size");
		const auto &center = c->lookup_variable("center");
		if (size.isDefined()) {
			bool converted=false;
			converted |= size.getDouble(node->x);
			converted |= size.getDouble(node->y);
			converted |= size.getVec2(node->x, node->y);
			if (!converted) {
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),"Unable to convert square(size=%1$s, ...) parameter to a number or a vec2 of numbers",size.toEchoString());
			} else if (OpenSCAD::rangeCheck) {
				bool ok = true;
				ok &= (node->x > 0) && (node->y > 0);
				ok &= std::isfinite(node->x) && std::isfinite(node->y);
				if(!ok){
					LOG(message_group::Warning,inst->location(),ctx->documentPath(),"square(size=%1$s, ...)",size.toEchoString());
				}
			}
		}
		if (center.type() == Value::Type::BOOL) {
			node->center = center.toBool();
		}
		break;
	}
	case primitive_type_e::CIRCLE: {
		const auto r = lookup_radius(c.ctx, inst->location(), "d", "r");
		if (r.type() == Value::Type::NUMBER) {
			node->r1 = r.toDouble();
			if (OpenSCAD::rangeCheck && ((node->r1 <= 0) || !std::isfinite(node->r1))){
				LOG(message_group::Warning,inst->location(),ctx->documentPath(),
					"circle(r=%1$s)",r.toEchoString());
			}
		}
		break;
	}
	case primitive_type_e::POLYGON: {
		node->points = c->lookup_variable("points").clone();
		node->paths = c->lookup_variable("paths").clone();
		break;
	}
	}

	node->convexity = (int)c->lookup_variable("convexity", true).toDouble();
	if (node->convexity < 1) node->convexity = 1;

	return node;
}

struct point2d {
	double x, y;
};

static void generate_circle(point2d *circle, double r, int fragments)
{
	for (int i=0; i<fragments; ++i) {
		double phi = (360.0 * i) / fragments;
		circle[i].x = r * cos_degrees(phi);
		circle[i].y = r * sin_degrees(phi);
	}
}

/*!
	Creates geometry for this node.
	May return an empty Geometry creation failed, but will not return nullptr.
*/
const Geometry *PrimitiveNode::createGeometry() const
{
	Geometry *g = nullptr;

	switch (this->type) {
	case primitive_type_e::CUBE: {
		auto p = new PolySet(3,true);
		g = p;
		if (this->x > 0 && this->y > 0 && this->z > 0 &&
			!std::isinf(this->x) && !std::isinf(this->y) && !std::isinf(this->z)) {
			double x1, x2, y1, y2, z1, z2;
			if (this->center) {
				x1 = -this->x/2;
				x2 = +this->x/2;
				y1 = -this->y/2;
				y2 = +this->y/2;
				z1 = -this->z/2;
				z2 = +this->z/2;
			} else {
				x1 = y1 = z1 = 0;
				x2 = this->x;
				y2 = this->y;
				z2 = this->z;
			}

			p->append_poly(); // top
			p->append_vertex(x1, y1, z2);
			p->append_vertex(x2, y1, z2);
			p->append_vertex(x2, y2, z2);
			p->append_vertex(x1, y2, z2);

			p->append_poly(); // bottom
			p->append_vertex(x1, y2, z1);
			p->append_vertex(x2, y2, z1);
			p->append_vertex(x2, y1, z1);
			p->append_vertex(x1, y1, z1);

			p->append_poly(); // side1
			p->append_vertex(x1, y1, z1);
			p->append_vertex(x2, y1, z1);
			p->append_vertex(x2, y1, z2);
			p->append_vertex(x1, y1, z2);

			p->append_poly(); // side2
			p->append_vertex(x2, y1, z1);
			p->append_vertex(x2, y2, z1);
			p->append_vertex(x2, y2, z2);
			p->append_vertex(x2, y1, z2);

			p->append_poly(); // side3
			p->append_vertex(x2, y2, z1);
			p->append_vertex(x1, y2, z1);
			p->append_vertex(x1, y2, z2);
			p->append_vertex(x2, y2, z2);

			p->append_poly(); // side4
			p->append_vertex(x1, y2, z1);
			p->append_vertex(x1, y1, z1);
			p->append_vertex(x1, y1, z2);
			p->append_vertex(x1, y2, z2);
		}
	}
		break;
	case primitive_type_e::SPHERE: {
		auto p = new PolySet(3,true);
		g = p;
		if (this->r1 > 0 && !std::isinf(this->r1)) {
			struct ring_s {
				std::vector<point2d> points;
				double z;
			};

			auto fragments = Calc::get_fragments_from_r(r1, fn, fs, fa);
			int rings = (fragments+1)/2;
// Uncomment the following three lines to enable experimental sphere tesselation
//		if (rings % 2 == 0) rings++; // To ensure that the middle ring is at phi == 0 degrees

			auto ring = std::vector<ring_s>(rings);

//		double offset = 0.5 * ((fragments / 2) % 2);
			for (int i = 0; i < rings; ++i) {
//			double phi = (180.0 * (i + offset)) / (fragments/2);
				double phi = (180.0 * (i + 0.5)) / rings;
				double r = r1 * sin_degrees(phi);
				ring[i].z = r1 * cos_degrees(phi);
				ring[i].points.resize(fragments);
				generate_circle(ring[i].points.data(), r, fragments);
			}

			p->append_poly();
			for (int i = 0; i < fragments; ++i)
				p->append_vertex(ring[0].points[i].x, ring[0].points[i].y, ring[0].z);

			for (int i = 0; i < rings-1; ++i) {
				auto r1 = &ring[i];
				auto  r2 = &ring[i+1];
				int r1i = 0, r2i = 0;
				while (r1i < fragments || r2i < fragments) {
					if (r1i >= fragments) goto sphere_next_r2;
					if (r2i >= fragments) goto sphere_next_r1;
					if ((double)r1i / fragments < (double)r2i / fragments) {
					sphere_next_r1:
						p->append_poly();
						int r1j = (r1i+1) % fragments;
						p->insert_vertex(r1->points[r1i].x, r1->points[r1i].y, r1->z);
						p->insert_vertex(r1->points[r1j].x, r1->points[r1j].y, r1->z);
						p->insert_vertex(r2->points[r2i % fragments].x, r2->points[r2i % fragments].y, r2->z);
						r1i++;
					} else {
					sphere_next_r2:
						p->append_poly();
						int r2j = (r2i+1) % fragments;
						p->append_vertex(r2->points[r2i].x, r2->points[r2i].y, r2->z);
						p->append_vertex(r2->points[r2j].x, r2->points[r2j].y, r2->z);
						p->append_vertex(r1->points[r1i % fragments].x, r1->points[r1i % fragments].y, r1->z);
						r2i++;
					}
				}
			}

			p->append_poly();
			for (int i = 0; i < fragments; ++i) {
				p->insert_vertex(ring[rings-1].points[i].x,
												 ring[rings-1].points[i].y,
												 ring[rings-1].z);
			}
		}
	}
		break;
	case primitive_type_e::CYLINDER: {
		auto p = new PolySet(3,true);
		g = p;
		if (this->h > 0 && !std::isinf(this->h) &&
				this->r1 >=0 && this->r2 >= 0 && (this->r1 > 0 || this->r2 > 0) &&
				!std::isinf(this->r1) && !std::isinf(this->r2)) {
			auto fragments = Calc::get_fragments_from_r(std::fmax(this->r1, this->r2), this->fn, this->fs, this->fa);

			double z1, z2;
			if (this->center) {
				z1 = -this->h/2;
				z2 = +this->h/2;
			} else {
				z1 = 0;
				z2 = this->h;
			}

			auto circle1 = std::vector<point2d>(fragments);
			auto circle2 = std::vector<point2d>(fragments);

			generate_circle(circle1.data(), r1, fragments);
			generate_circle(circle2.data(), r2, fragments);

			for (int i=0; i<fragments; ++i) {
				int j = (i+1) % fragments;
				if (r1 == r2) {
					p->append_poly();
					p->insert_vertex(circle1[i].x, circle1[i].y, z1);
					p->insert_vertex(circle2[i].x, circle2[i].y, z2);
					p->insert_vertex(circle2[j].x, circle2[j].y, z2);
					p->insert_vertex(circle1[j].x, circle1[j].y, z1);
				} else {
					if (r1 > 0) {
						p->append_poly();
						p->insert_vertex(circle1[i].x, circle1[i].y, z1);
						p->insert_vertex(circle2[i].x, circle2[i].y, z2);
						p->insert_vertex(circle1[j].x, circle1[j].y, z1);
					}
					if (r2 > 0) {
						p->append_poly();
						p->insert_vertex(circle2[i].x, circle2[i].y, z2);
						p->insert_vertex(circle2[j].x, circle2[j].y, z2);
						p->insert_vertex(circle1[j].x, circle1[j].y, z1);
					}
				}
			}

			if (this->r1 > 0) {
				p->append_poly();
				for (int i=0; i<fragments; ++i)
					p->insert_vertex(circle1[i].x, circle1[i].y, z1);
			}

			if (this->r2 > 0) {
				p->append_poly();
				for (int i=0; i<fragments; ++i)
					p->append_vertex(circle2[i].x, circle2[i].y, z2);
			}
		}
	}
		break;
	case primitive_type_e::POLYHEDRON: {
		auto p = new PolySet(3);
		g = p;
		p->setConvexity(this->convexity);
		const auto &pts = this->points.toVector();
		size_t face_i = 0;
		for (const auto &face : this->faces.toVector())	{
			p->append_poly();
			size_t fp_i = 0;
			for (const auto &pt_i_val : face.toVector()) {
				size_t pt_i = (size_t)pt_i_val.toDouble();
				if (pt_i < pts.size()) {
					double px, py, pz;
					if (!pts[pt_i].getVec3(px, py, pz, 0.0) ||
					    !std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz)) {
						LOG(message_group::Error,this->modinst->location(),this->document_path,"Unable to convert points[%1$d] = %2$s to a vec3 of numbers",pt_i,pts[pt_i].toEchoString());
						return p;
					}
					p->insert_vertex(px, py, pz);
				} else {
					LOG(message_group::Warning,this->modinst->location(),this->document_path,"Point index %1$d is out of bounds (from faces[%2$d][%3$d])",pt_i,face_i,fp_i);
				}
				++fp_i;
			}
			++face_i;
		}
	}
		break;
	case primitive_type_e::SQUARE: {
		auto p = new Polygon2d();
		g = p;
		if (this->x > 0 && this->y > 0 &&
				!std::isinf(this->x) && !std::isinf(this->y)) {
			Vector2d v1(0, 0);
			Vector2d v2(this->x, this->y);
			if (this->center) {
				v1 -= Vector2d(this->x/2, this->y/2);
				v2 -= Vector2d(this->x/2, this->y/2);
			}

			Outline2d o;
			o.vertices = {v1, {v2[0], v1[1]}, v2, {v1[0], v2[1]}};
			p->addOutline(o);
		}
		p->setSanitized(true);
	}
		break;
	case primitive_type_e::CIRCLE: {
		auto p = new Polygon2d();
		g = p;
		if (this->r1 > 0 && !std::isinf(this->r1))	{
			auto fragments = Calc::get_fragments_from_r(this->r1, this->fn, this->fs, this->fa);

			Outline2d o;
			o.vertices.resize(fragments);
			for (int i=0; i < fragments; ++i) {
				double phi = (360.0 * i) / fragments;
				o.vertices[i] = {this->r1 * cos_degrees(phi), this->r1 * sin_degrees(phi)};
			}
			p->addOutline(o);
		}
		p->setSanitized(true);
	}
		break;
	case primitive_type_e::POLYGON:	{
			auto p = new Polygon2d();
			g = p;
			Outline2d outline;
			double x,y;
			size_t i = 0;
			for (const auto &val : this->points.toVector()) {
				if (!val.getVec2(x, y) || std::isinf(x) || std::isinf(y)) {
					LOG(message_group::Error,this->modinst->location(),this->document_path,"Unable to convert points[%1$d] = %2$s to a vec2 of numbers",i,val.toEchoString());
					return p;
				}
				outline.vertices.emplace_back(x, y);
				++i;
			}

			if (this->paths.toVector().size() == 0 && outline.vertices.size() > 2) {
				p->addOutline(outline);
			}
			else {
				size_t path_i = 0;
				for (const auto &polygon : this->paths.toVector()) {
					Outline2d curroutline;
					size_t path_pt_i = 0;
					for (const auto &index : polygon.toVector()) {
						unsigned int idx = (unsigned int)index.toDouble();
						if (idx < outline.vertices.size()) {
							curroutline.vertices.push_back(outline.vertices[idx]);
						} else {
							LOG(message_group::Warning,this->modinst->location(),this->document_path,"Point index %1$d is out of bounds (from paths[%2$d][%3$d])",idx , path_i , path_pt_i);
						}
						++path_pt_i;
					}
					p->addOutline(curroutline);
					++path_i;
				}
			}

			if (p->outlines().size() > 0) {
				p->setConvexity(convexity);
			}
		}
	}

	return g;
}

std::string PrimitiveNode::toString() const
{
	std::ostringstream stream;

	stream << this->name();

	switch (this->type) {
	case primitive_type_e::CUBE:
		stream << "(size = [" << this->x << ", " << this->y << ", " << this->z << "], "
					 <<	"center = " << (center ? "true" : "false") << ")";
		break;
	case primitive_type_e::SPHERE:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", r = " << this->r1 << ")";
			break;
	case primitive_type_e::CYLINDER:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", h = " << this->h << ", r1 = " << this->r1
					 << ", r2 = " << this->r2 << ", center = " << (center ? "true" : "false") << ")";
			break;
	case primitive_type_e::POLYHEDRON:
		stream << "(points = ";
		this->points.toStream(stream);
		stream << ", faces = ";
		this->faces.toStream(stream);
		stream << ", convexity = " << this->convexity << ")";
			break;
	case primitive_type_e::SQUARE:
		stream << "(size = [" << this->x << ", " << this->y << "], "
					 << "center = " << (center ? "true" : "false") << ")";
			break;
	case primitive_type_e::CIRCLE:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", r = " << this->r1 << ")";
		break;
	case primitive_type_e::POLYGON:
		stream << "(points = ";
		this->points.toStream(stream);
		stream << ", paths = ";
		this->paths.toStream(stream);
		stream << ", convexity = " << this->convexity << ")";
			break;
	default:
		assert(false);
	}

	return stream.str();
}

void register_builtin_primitives()
{
	Builtins::init("cube" , new PrimitiveModule(primitive_type_e::CUBE),
				{
					"cube(size)",
					"cube([width, depth, height])",
					"cube([width, depth, height], center = true)",
				});

	Builtins::init("sphere", new PrimitiveModule(primitive_type_e::SPHERE),
				{
					"sphere(radius)",
					"sphere(r = radius)",
					"sphere(d = diameter)",
				});

	Builtins::init("cylinder", new PrimitiveModule(primitive_type_e::CYLINDER),
				{
					"cylinder(h, r1, r2)",
					"cylinder(h = height, r = radius, center = true)",
					"cylinder(h = height, r1 = bottom, r2 = top, center = true)",
					"cylinder(h = height, d = diameter, center = true)",
					"cylinder(h = height, d1 = bottom, d2 = top, center = true)",
				});

	Builtins::init("polyhedron", new PrimitiveModule(primitive_type_e::POLYHEDRON),
				{
					"polyhedron(points, faces, convexity)",
				});

	Builtins::init("square", new PrimitiveModule(primitive_type_e::SQUARE),
				{
					"square(size, center = true)",
					"square([width,height], center = true)",
				});

	Builtins::init("circle", new PrimitiveModule(primitive_type_e::CIRCLE),
				{
					"circle(radius)",
					"circle(r = radius)",
					"circle(d = diameter)",
				});

	Builtins::init("polygon", new PrimitiveModule(primitive_type_e::POLYGON),
				{
					"polygon([points])",
					"polygon([points], [paths])",
				});
}

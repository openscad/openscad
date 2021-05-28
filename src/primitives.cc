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
#include "children.h"
#include "Polygon2d.h"
#include "builtin.h"
#include "parameters.h"
#include "printutils.h"
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

struct point2d {
	double x, y;
};

struct point3d {
	double x, y, z;
};

static void generate_circle(point2d *circle, double r, int fragments)
{
	for (int i=0; i<fragments; ++i) {
		double phi = (360.0 * i) / fragments;
		circle[i].x = r * cos_degrees(phi);
		circle[i].y = r * sin_degrees(phi);
	}
}

/**
 * Return a radius value by looking up both a diameter and radius variable.
 * The diameter has higher priority, so if found an additionally set radius
 * value is ignored.
 *
 * @param parameters parameters with variable values.
 * @param inst containing instantiation.
 * @param radius_var name of the variable to lookup for the radius value.
 * @param diameter_var name of the variable to lookup for the diameter value.
 * @return radius value of type Value::Type::NUMBER or Value::Type::UNDEFINED if both
 *         variables are invalid or not set.
 */
static Value lookup_radius(const Parameters& parameters, const ModuleInstantiation *inst, const std::string &diameter_var, const std::string &radius_var)
{
	const auto &d = parameters[diameter_var];
	const auto &r = parameters[radius_var];
	const auto r_defined = (r.type() == Value::Type::NUMBER);

	if (d.type() == Value::Type::NUMBER) {
		if (r_defined) {
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
				"Ignoring radius variable '%1$s' as diameter '%2$s' is defined too.",radius_var,diameter_var);
		}
		return d.toDouble() / 2.0;
	} else if (r_defined) {
		return r.clone();
	} else {
		return Value::undefined.clone();
	}
}

static void set_fragments(const Parameters& parameters, const ModuleInstantiation *inst, double& fn, double& fs, double& fa)
{
	fn = parameters["$fn"].toDouble();
	fs = parameters["$fs"].toDouble();
	fa = parameters["$fa"].toDouble();

	if (fs < F_MINIMUM) {
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
			"$fs too small - clamping to %1$f",F_MINIMUM);
		fs = F_MINIMUM;
	}
	if (fa < F_MINIMUM) {
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
			"$fa too small - clamping to %1$f",F_MINIMUM);
		fa = F_MINIMUM;
	}
}



class CubeNode : public LeafNode
{
public:
	CubeNode(const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override
	{
		std::ostringstream stream;
		stream << "cube(size = ["
			<< x << ", "
			<< y << ", "
			<< z << "], center = "
			<< (center ? "true" : "false") << ")";
		return stream.str();
	}
	std::string name() const override { return "cube"; }
	const Geometry *createGeometry() const override;
	
	double x = 1, y = 1, z = 1;
	bool center = false;
};

const Geometry* CubeNode::createGeometry() const
{
	auto p = new PolySet(3,true);
	if (
		   this->x <= 0 || !std::isfinite(this->x)
		|| this->y <= 0 || !std::isfinite(this->y)
		|| this->z <= 0 || !std::isfinite(this->z)
	) {
		return p;
	}

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

	return p;
}

static AbstractNode* builtin_cube(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new CubeNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

	const auto &size = parameters["size"];
	if (size.isDefined()) {
		bool converted=false;
		converted |= size.getDouble(node->x);
		converted |= size.getDouble(node->y);
		converted |= size.getDouble(node->z);
		converted |= size.getVec3(node->x, node->y, node->z);
		if (!converted) {
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"Unable to convert cube(size=%1$s, ...) parameter to a number or a vec3 of numbers",size.toEchoString());
		} else if(OpenSCAD::rangeCheck) {
			bool ok = (node->x > 0) && (node->y > 0) && (node->z > 0);
			ok &= std::isfinite(node->x) && std::isfinite(node->y) && std::isfinite(node->z);
			if(!ok){
				LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"cube(size=%1$s, ...)",size.toEchoString());
			}
		}
	}
	if (parameters["center"].type() == Value::Type::BOOL) {
		node->center = parameters["center"].toBool();
	}

	return node;
}



class SphereNode : public LeafNode
{
public:
	SphereNode(const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override
	{
		std::ostringstream stream;
		stream << "sphere"
			<< "($fn = " << fn
			<< ", $fa = " << fa
			<< ", $fs = " << fs
			<< ", r = " << r
			<< ")";
		return stream.str();
	}
	std::string name() const override { return "sphere"; }
	const Geometry *createGeometry() const override;
	
	double fn, fs, fa;
	double r = 1;
};

const Geometry* SphereNode::createGeometry() const
{
	auto p = new PolySet(3,true);
	if (this->r <= 0 || !std::isfinite(this->r)) {
		return p;
	}

	struct ring_s {
		std::vector<point2d> points;
		double z;
	};

	auto fragments = Calc::get_fragments_from_r(r, fn, fs, fa);
	int rings = (fragments+1)/2;
// Uncomment the following three lines to enable experimental sphere tesselation
//	if (rings % 2 == 0) rings++; // To ensure that the middle ring is at phi == 0 degrees

	auto ring = std::vector<ring_s>(rings);

//	double offset = 0.5 * ((fragments / 2) % 2);
	for (int i = 0; i < rings; ++i) {
//		double phi = (180.0 * (i + offset)) / (fragments/2);
		double phi = (180.0 * (i + 0.5)) / rings;
		double radius = r * sin_degrees(phi);
		ring[i].z = r * cos_degrees(phi);
		ring[i].points.resize(fragments);
		generate_circle(ring[i].points.data(), radius, fragments);
	}

	p->append_poly();
	for (int i = 0; i < fragments; ++i)
		p->append_vertex(ring[0].points[i].x, ring[0].points[i].y, ring[0].z);

	for (int i = 0; i < rings-1; ++i) {
		auto r1 = &ring[i];
		auto r2 = &ring[i+1];
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
		p->insert_vertex(
			ring[rings-1].points[i].x,
			ring[rings-1].points[i].y,
			ring[rings-1].z
		);
	}

	return p;
}

static AbstractNode* builtin_sphere(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new SphereNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d"});

	set_fragments(parameters, inst, node->fn, node->fs, node->fa);
	const auto r = lookup_radius(parameters, inst, "d", "r");
	if (r.type() == Value::Type::NUMBER) {
		node->r = r.toDouble();
		if (OpenSCAD::rangeCheck && (node->r <= 0 || !std::isfinite(node->r))){
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
				"sphere(r=%1$s)",r.toEchoString());
		}
	}

	return node;
}



class CylinderNode : public LeafNode
{
public:
	CylinderNode(const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override
	{
		std::ostringstream stream;
		stream << "cylinder"
			<< "($fn = " << fn
			<< ", $fa = " << fa
			<< ", $fs = " << fs
			<< ", h = " << h
			<< ", r1 = " << r1
			<< ", r2 = " << r2
			<< ", center = " << (center ? "true" : "false")
			<< ")";
		return stream.str();
	}
	std::string name() const override { return "cylinder"; }
	const Geometry *createGeometry() const override;
	
	double fn, fs, fa;
	double r1 = 1, r2 = 1, h = 1;
	bool center = false;
};

const Geometry* CylinderNode::createGeometry() const
{
	auto p = new PolySet(3,true);
	if (
		   this->h <= 0 || !std::isfinite(this->h)
		|| this->r1 < 0 || !std::isfinite(this->r1)
		|| this->r2 < 0 || !std::isfinite(this->r2)
		|| (this->r1 <= 0 && this->r2 <= 0)
	) {
		return p;
	}

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

	return p;
}

static AbstractNode* builtin_cylinder(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new CylinderNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"h", "r1", "r2", "center"}, {"r", "d", "d1", "d2"});

	set_fragments(parameters, inst, node->fn, node->fs, node->fa);
	if (parameters["h"].type() == Value::Type::NUMBER) {
		node->h = parameters["h"].toDouble();
	}

	auto r = lookup_radius(parameters, inst, "d", "r");
	auto r1 = lookup_radius(parameters, inst, "d1", "r1");
	auto r2 = lookup_radius(parameters, inst, "d2", "r2");
	if (r.type() == Value::Type::NUMBER &&
		(r1.type() == Value::Type::NUMBER || r2.type() == Value::Type::NUMBER)
	) {
		LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"Cylinder parameters ambiguous");
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
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"cylinder(h=%1$s, ...)",parameters["h"].toEchoString());
		}
		if (node->r1 < 0 || node->r2 < 0 || (node->r1 == 0 && node->r2 == 0) || !std::isfinite(node->r1) || !std::isfinite(node->r2)){
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
				"cylinder(r1=%1$s, r2=%2$s, ...)",
				(r1.type() == Value::Type::NUMBER ? r1.toEchoString() : r.toEchoString()),
				(r2.type() == Value::Type::NUMBER ? r2.toEchoString() : r.toEchoString()));
		}
	}

	if (parameters["center"].type() == Value::Type::BOOL) {
		node->center = parameters["center"].toBool();
	}

	return node;
}



class PolyhedronNode : public LeafNode
{
public:
	PolyhedronNode (const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override;
	std::string name() const override { return "polyhedron"; }
	const Geometry *createGeometry() const override;
	
	std::vector<point3d> points;
	std::vector<std::vector<size_t>> faces;
	int convexity = 1;
};

std::string PolyhedronNode::toString() const
{
	std::ostringstream stream;
	stream << "polyhedron(points = [";
	bool firstPoint = true;
	for (const auto& point : this->points) {
		if (firstPoint) {
			firstPoint = false;
		} else {
			stream << ", ";
		}
		stream << "[" << point.x << ", " << point.y << ", " << point.z << "]";
	}
	stream << "], faces = [";
	bool firstFace = true;
	for (const auto& face : this->faces) {
		if (firstFace) {
			firstFace = false;
		} else {
			stream << ", ";
		}
		stream << "[";
		bool firstIndex = true;
		for (const auto& index : face) {
			if (firstIndex) {
				firstIndex = false;
			} else {
				stream << ", ";
			}
			stream << index;
		}
		stream << "]";
	}
	stream << "], convexity = " << this->convexity << ")";
	return stream.str();
}

const Geometry* PolyhedronNode::createGeometry() const
{
	auto p = new PolySet(3);
	p->setConvexity(this->convexity);
	for (const auto& face : this->faces) {
		p->append_poly();
		for (const auto& index : face) {
			assert(index < this->points.size());
			const auto& point = points[index];
			p->insert_vertex(point.x, point.y, point.z);
		}
	}
	return p;
}

static AbstractNode* builtin_polyhedron(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new PolyhedronNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"points", "faces", "convexity"}, {"triangles"});

	if (parameters["points"].type() != Value::Type::VECTOR) {
		LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert points = %1$s to a vector of coordinates",parameters["points"].toEchoString());
		return node;
	}
	for (const Value& pointValue : parameters["points"].toVector()) {
		point3d point;
		if (!pointValue.getVec3(point.x, point.y, point.z, 0.0) ||
			!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)
		) {
			LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert points[%1$d] = %2$s to a vec3 of numbers",node->points.size(),pointValue.toEchoString());
			node->points.push_back({0, 0, 0});
		} else {
			node->points.push_back(point);
		}
	}

	const Value* faces = nullptr;
	if (parameters["faces"].type() == Value::Type::UNDEFINED && parameters["triangles"].type() != Value::Type::UNDEFINED) {
		// backwards compatible
		LOG(message_group::Deprecated,inst->location(),parameters.documentRoot(),"polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
		faces = &parameters["triangles"];
	} else {
		faces = &parameters["faces"];
	}
	if (faces->type() != Value::Type::VECTOR) {
		LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert faces = %1$s to a vector of vector of point indices",faces->toEchoString());
		return node;
	}
	size_t faceIndex = 0;
	for (const Value& faceValue : faces->toVector()) {
		if (faceValue.type() != Value::Type::VECTOR) {
			LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert faces[%1$d] = %2$s to a vector of numbers",faceIndex,faceValue.toEchoString());
		} else {
			size_t pointIndexIndex = 0;
			std::vector<size_t> face;
			for (const Value& pointIndexValue : faceValue.toVector()) {
				if (pointIndexValue.type() != Value::Type::NUMBER) {
					LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert faces[%1$d][%2$d] = %3$s to a number",faceIndex,pointIndexIndex,pointIndexValue.toEchoString());
				} else {
					size_t pointIndex = (size_t)pointIndexValue.toDouble();
					if (pointIndex < node->points.size()) {
						face.push_back(pointIndex);
					} else {
						LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"Point index %1$d is out of bounds (from faces[%2$d][%3$d])",pointIndex,faceIndex,pointIndexIndex);
					}
				}
				pointIndexIndex++;
			}
			node->faces.push_back(std::move(face));
		}
		faceIndex++;
	}

	node->convexity = (int)parameters["convexity"].toDouble();
	if (node->convexity < 1) node->convexity = 1;

	return node;
}



class SquareNode : public LeafNode
{
public:
	SquareNode(const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override
	{
		std::ostringstream stream;
		stream << "square(size = ["
			<< x << ", "
			<< y << "], center = "
			<< (center ? "true" : "false") << ")";
		return stream.str();
	}
	std::string name() const override { return "square"; }
	const Geometry *createGeometry() const override;
	
	double x = 1, y = 1;
	bool center = false;
};

const Geometry* SquareNode::createGeometry() const
{
	auto p = new Polygon2d();
	if (
		   this->x <= 0 || !std::isfinite(this->x)
		|| this->y <= 0 || !std::isfinite(this->y)
	) {
		return p;
	}

	Vector2d v1(0, 0);
	Vector2d v2(this->x, this->y);
	if (this->center) {
		v1 -= Vector2d(this->x/2, this->y/2);
		v2 -= Vector2d(this->x/2, this->y/2);
	}

	Outline2d o;
	o.vertices = {v1, {v2[0], v1[1]}, v2, {v1[0], v2[1]}};
	p->addOutline(o);
	p->setSanitized(true);
	return p;
}

static AbstractNode* builtin_square(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new SquareNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

	const auto &size = parameters["size"];
	if (size.isDefined()) {
		bool converted=false;
		converted |= size.getDouble(node->x);
		converted |= size.getDouble(node->y);
		converted |= size.getVec2(node->x, node->y);
		if (!converted) {
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"Unable to convert square(size=%1$s, ...) parameter to a number or a vec2 of numbers",size.toEchoString());
		} else if (OpenSCAD::rangeCheck) {
			bool ok = true;
			ok &= (node->x > 0) && (node->y > 0);
			ok &= std::isfinite(node->x) && std::isfinite(node->y);
			if(!ok){
				LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"square(size=%1$s, ...)",size.toEchoString());
			}
		}
	}
	if (parameters["center"].type() == Value::Type::BOOL) {
		node->center = parameters["center"].toBool();
	}

	return node;
}




class CircleNode : public LeafNode
{
public:
	CircleNode(const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override
	{
		std::ostringstream stream;
		stream << "circle"
			<< "($fn = " << fn
			<< ", $fa = " << fa
			<< ", $fs = " << fs
			<< ", r = " << r
			<< ")";
		return stream.str();
	}
	std::string name() const override { return "circle"; }
	const Geometry *createGeometry() const override;
	
	double fn, fs, fa;
	double r = 1;
};

const Geometry* CircleNode::createGeometry() const
{
	auto p = new Polygon2d();
	if (this->r <= 0 || !std::isfinite(this->r)) {
		return p;
	}

	auto fragments = Calc::get_fragments_from_r(this->r, this->fn, this->fs, this->fa);
	Outline2d o;
	o.vertices.resize(fragments);
	for (int i=0; i < fragments; ++i) {
		double phi = (360.0 * i) / fragments;
		o.vertices[i] = {this->r * cos_degrees(phi), this->r * sin_degrees(phi)};
	}
	p->addOutline(o);
	p->setSanitized(true);
	return p;
}

static AbstractNode* builtin_circle(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new CircleNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d"});

	set_fragments(parameters, inst, node->fn, node->fs, node->fa);
	const auto r = lookup_radius(parameters, inst, "d", "r");
	if (r.type() == Value::Type::NUMBER) {
		node->r = r.toDouble();
		if (OpenSCAD::rangeCheck && ((node->r <= 0) || !std::isfinite(node->r))){
			LOG(message_group::Warning,inst->location(),parameters.documentRoot(),
				"circle(r=%1$s)",r.toEchoString());
		}
	}

	return node;
}



class PolygonNode : public LeafNode
{
public:
	PolygonNode (const ModuleInstantiation *mi): LeafNode(mi) {}
	std::string toString() const override;
	std::string name() const override { return "polygon"; }
	const Geometry *createGeometry() const override;
	
	std::vector<point2d> points;
	std::vector<std::vector<size_t>> paths;
	int convexity = 1;
};

std::string PolygonNode::toString() const
{
	std::ostringstream stream;
	stream << "polygon(points = [";
	bool firstPoint = true;
	for (const auto& point : this->points) {
		if (firstPoint) {
			firstPoint = false;
		} else {
			stream << ", ";
		}
		stream << "[" << point.x << ", " << point.y << "]";
	}
	stream << "], paths = ";
	if (this->paths.empty()) {
		stream << "undef";
	} else {
		stream << "[";
		bool firstPath = true;
		for (const auto& path : this->paths) {
			if (firstPath) {
				firstPath = false;
			} else {
				stream << ", ";
			}
			stream << "[";
			bool firstIndex = true;
			for (const auto& index : path) {
				if (firstIndex) {
					firstIndex = false;
				} else {
					stream << ", ";
				}
				stream << index;
			}
			stream << "]";
		}
		stream << "]";
	}
	stream << ", convexity = " << this->convexity << ")";
	return stream.str();
}

const Geometry* PolygonNode::createGeometry() const
{
	auto p = new Polygon2d();
	if (this->paths.empty() && this->points.size() > 2) {
		Outline2d outline;
		for (const auto& point : this->points) {
			outline.vertices.emplace_back(point.x, point.y);
		}
		p->addOutline(outline);
	} else {
		for (const auto& path : this->paths) {
			Outline2d outline;
			for (const auto& index : path) {
				assert(index < this->points.size());
				const auto& point = points[index];
				outline.vertices.emplace_back(point.x, point.y);
			}
			p->addOutline(outline);
		}
	}
	if (p->outlines().size() > 0) {
		p->setConvexity(convexity);
	}
	return p;
}

static AbstractNode* builtin_polygon(const ModuleInstantiation *inst, Arguments arguments, Children children)
{
	auto node = new PolygonNode(inst);

	if (!children.empty()) {
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
			"module %1$s() does not support child modules",node->name());
	}

	Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"points", "paths", "convexity"});

	if (parameters["points"].type() != Value::Type::VECTOR) {
		LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert points = %1$s to a vector of coordinates",parameters["points"].toEchoString());
		return node;
	}
	for (const Value& pointValue : parameters["points"].toVector()) {
		point2d point;
		if (!pointValue.getVec2(point.x, point.y) ||
			!std::isfinite(point.x) || !std::isfinite(point.y)
		) {
			LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert points[%1$d] = %2$s to a vec2 of numbers",node->points.size(),pointValue.toEchoString());
			node->points.push_back({0, 0});
		} else {
			node->points.push_back(point);
		}
	}

	if (parameters["paths"].type() == Value::Type::VECTOR) {
		size_t pathIndex = 0;
		for (const Value& pathValue : parameters["paths"].toVector()) {
			if (pathValue.type() != Value::Type::VECTOR) {
				LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert paths[%1$d] = %2$s to a vector of numbers",pathIndex,pathValue.toEchoString());
			} else {
				size_t pointIndexIndex = 0;
				std::vector<size_t> path;
				for (const Value& pointIndexValue : pathValue.toVector()) {
					if (pointIndexValue.type() != Value::Type::NUMBER) {
						LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert paths[%1$d][%2$d] = %3$s to a number",pathIndex,pointIndexIndex,pointIndexValue.toEchoString());
					} else {
						size_t pointIndex = (size_t)pointIndexValue.toDouble();
						if (pointIndex < node->points.size()) {
							path.push_back(pointIndex);
						} else {
							LOG(message_group::Warning,inst->location(),parameters.documentRoot(),"Point index %1$d is out of bounds (from paths[%2$d][%3$d])",pointIndex,pathIndex,pointIndexIndex);
						}
					}
					pointIndexIndex++;
				}
				node->paths.push_back(std::move(path));
			}
			pathIndex++;
		}
	} else if (parameters["paths"].type() != Value::Type::UNDEFINED) {
		LOG(message_group::Error,inst->location(),parameters.documentRoot(),"Unable to convert paths = %1$s to a vector of vector of point indices",parameters["paths"].toEchoString());
		return node;
	}

	node->convexity = (int)parameters["convexity"].toDouble();
	if (node->convexity < 1) node->convexity = 1;

	return node;
}



void register_builtin_primitives()
{
	Builtins::init("cube", new BuiltinModule(builtin_cube),
				{
					"cube(size)",
					"cube([width, depth, height])",
					"cube([width, depth, height], center = true)",
				});

	Builtins::init("sphere", new BuiltinModule(builtin_sphere),
				{
					"sphere(radius)",
					"sphere(r = radius)",
					"sphere(d = diameter)",
				});

	Builtins::init("cylinder", new BuiltinModule(builtin_cylinder),
				{
					"cylinder(h, r1, r2)",
					"cylinder(h = height, r = radius, center = true)",
					"cylinder(h = height, r1 = bottom, r2 = top, center = true)",
					"cylinder(h = height, d = diameter, center = true)",
					"cylinder(h = height, d1 = bottom, d2 = top, center = true)",
				});

	Builtins::init("polyhedron", new BuiltinModule(builtin_polyhedron),
				{
					"polyhedron(points, faces, convexity)",
				});

	Builtins::init("square", new BuiltinModule(builtin_square),
				{
					"square(size, center = true)",
					"square([width,height], center = true)",
				});

	Builtins::init("circle", new BuiltinModule(builtin_circle),
				{
					"circle(radius)",
					"circle(r = radius)",
					"circle(d = diameter)",
				});

	Builtins::init("polygon", new BuiltinModule(builtin_polygon),
				{
					"polygon([points])",
					"polygon([points], [paths])",
				});
}

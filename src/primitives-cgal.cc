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
#include "cgalutils.h"
#include "builtin.h"
#include "printutils.h"
#include "visitor.h"
#include "context.h"
#include "calc.h"
#include "mathc99.h"
#include <sstream>
#include <assert.h>
#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/math/special_functions/fpclassify.hpp>
#define isinf boost::math::isinf

#define F_MINIMUM 0.01

enum primitive_cgal_type_e {
    POINTSET
};

class PrimitiveCGALModule : public AbstractModule
{
public:
	primitive_cgal_type_e type;
	PrimitiveCGALModule(primitive_cgal_type_e type) : type(type) { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
private:
	Value lookup_radius(const Context &ctx, const std::string &radius_var, const std::string &diameter_var) const;
};

class PrimitiveCGALNode : public LeafNode
{
public:
	PrimitiveCGALNode(const ModuleInstantiation *mi, primitive_cgal_type_e type) : LeafNode(mi), type(type) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const {
		switch (this->type) {
		case POINTSET:
			return "pointset";
			break;
		default:
			assert(false && "PrimitiveCGALNode::name(): Unknown primitive type");
			return "unknown";
		}
	}

	bool center;
	double x, y, z, h, r1, r2;
	double fn, fs, fa;
	primitive_cgal_type_e type;
	int convexity;
	ValuePtr points, paths, faces;
	virtual Geometry *createGeometry() const;
    int neighbors;
    double scale_num_points, sharpness_angle, edge_sensitivity, neighbor_radius;
    double angle, radius, distance;
};

/**
 * Return a radius value by looking up both a diameter and radius variable.
 * The diameter has higher priority, so if found an additionally set radius
 * value is ignored.
 * 
 * @param ctx data context with variable values.
 * @param radius_var name of the variable to lookup for the radius value.
 * @param diameter_var name of the variable to lookup for the diameter value.
 * @return radius value of type Value::NUMBER or Value::UNDEFINED if both
 *         variables are invalid or not set.
 */
Value PrimitiveCGALModule::lookup_radius(const Context &ctx, const std::string &diameter_var, const std::string &radius_var) const
{
	ValuePtr d = ctx.lookup_variable(diameter_var, true);
	ValuePtr r = ctx.lookup_variable(radius_var, true);
	const bool r_defined = (r->type() == Value::NUMBER);
	
	if (d->type() == Value::NUMBER) {
		if (r_defined) {
			PRINTB("WARNING: Ignoring radius variable '%s' as diameter '%s' is defined too.", radius_var % diameter_var);
		}
		return Value(d->toDouble() / 2.0);
	} else if (r_defined) {
		return *r;
	} else {
		return Value::undefined;
	}
}

AbstractNode *PrimitiveCGALModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	PrimitiveCGALNode *node = new PrimitiveCGALNode(inst, this->type);

	node->center = false;
	node->x = node->y = node->z = node->h = node->r1 = node->r2 = 1;

	AssignmentList args;

	switch (this->type) {
	case POINTSET:
		args += Assignment("points")
            , Assignment("neighbors")
            , Assignment("scale_num_points"), Assignment("sharpness_angle"), Assignment("edge_sensitivity"), Assignment("neighbor_radius")
            , Assignment("angle"), Assignment("radius"), Assignment("distance")
            , Assignment("convexity");
		break;
	default:
		assert(false && "PrimitiveCGALModule::instantiate(): Unknown node type");
	}

	Context c(ctx);
	c.setVariables(args, evalctx);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	if (node->fs < F_MINIMUM) {
		PRINTB("WARNING: $fs too small - clamping to %f", F_MINIMUM);
		node->fs = F_MINIMUM;
	}
	if (node->fa < F_MINIMUM) {
		PRINTB("WARNING: $fa too small - clamping to %f", F_MINIMUM);
		node->fa = F_MINIMUM;
	}

	switch (this->type)  {
	case POINTSET: {
		node->points = c.lookup_variable("points");
        // jet_estimate_normals & mst_orient_normals 'neighbors' parameter
        ValuePtr neighbors = c.lookup_variable("neighbors");
        if (neighbors->type() == Value::NUMBER) {
            node->neighbors = neighbors->toDouble();
        } else {
            node->neighbors = 16;
        }
        if (node->neighbors < 0) node->neighbors = 0;
        if (node->neighbors > 0 && node->neighbors < 4 ) node->neighbors = 4;
        PRINTB("POINTSET neighbors: %d",node->neighbors);
        // edge_aware_upsample_point_set parameters
        ValuePtr scale_num_points = c.lookup_variable("scale_num_points");
        if ( scale_num_points->type() == Value::NUMBER) {
            const double snp = scale_num_points->toDouble();
            if( snp > 1.0 ) {
                node->scale_num_points = snp;
            } else {
                node->scale_num_points = 0;
            }
        } else {
            node->scale_num_points = 0;
        }
        PRINTB("POINTSET scale_num_points: %d",node->scale_num_points);
        ValuePtr sharpness_angle = c.lookup_variable("sharpness_angle");
        if ( sharpness_angle->type() == Value::NUMBER) {
            const double sat = sharpness_angle->toDouble();
            node->sharpness_angle = sat;
        } else {
            node->sharpness_angle = 25.0;
        }
        PRINTB("POINTSET sharpness_angle: %d",node->sharpness_angle);
        ValuePtr edge_sensitivity = c.lookup_variable("edge_sensitivity");
        if ( edge_sensitivity->type() == Value::NUMBER) {
            const double es = edge_sensitivity->toDouble();
            if( es < 0.0 ) {
                node->edge_sensitivity = 0.0;
            } else if( es > 1.0 ) {
                node->edge_sensitivity = 1.0;
            } else {
                node->edge_sensitivity = es;
            }
        } else {
            node->edge_sensitivity = 0.0;
        }
        PRINTB("POINTSET edge_sensitivity: %d",node->edge_sensitivity);
        // neighbor_radius = 0.25
        ValuePtr neighbor_radius = c.lookup_variable("neighbor_radius");
        if ( neighbor_radius->type() == Value::NUMBER) {
            const double nr = neighbor_radius->toDouble();
            if( nr > 0 ) {
                node->neighbor_radius = nr;
            } else {
                node->neighbor_radius = 0.25;
            }
        } else {
            node->neighbor_radius = 0.25;
        }
        PRINTB("POINTSET neighbor_radius: %d",node->neighbor_radius);
        // Surface_mesh_default_criteria_3 parameters
        ValuePtr angle = c.lookup_variable("angle");
        if (angle->type() == Value::NUMBER) {
            node->angle = angle->toDouble();
        } else {
            node->angle = 20.0;
        }
        PRINTB("POINTSET angle: %d",node->angle);
        ValuePtr radius = c.lookup_variable("radius");
        if (radius->type() == Value::NUMBER) {
            node->radius = radius->toDouble();
        } else {
            node->radius = 30;
        }
        PRINTB("POINTSET radius: %d",node->radius);
        ValuePtr distance = c.lookup_variable("distance");
        if (distance->type() == Value::NUMBER) {
            node->distance = distance->toDouble();
        } else {
            node->distance = 0.05;
        }
        PRINTB("POINTSET distance: %d",node->distance);
		break;
	}
	}

	node->convexity = c.lookup_variable("convexity", true)->toDouble();
	if (node->convexity < 1)
		node->convexity = 1;

	return node;
}

struct point2d {
	double x, y;
};

static void generate_circle(point2d *circle, double r, int fragments)
{
	for (int i=0; i<fragments; i++) {
		double phi = (M_PI*2*i) / fragments;
		circle[i].x = r*cos(phi);
		circle[i].y = r*sin(phi);
	}
}

/*!
	Creates geometry for this node.
	May return an empty Geometry creation failed, but will not return NULL.
*/
Geometry *PrimitiveCGALNode::createGeometry() const
{
	Geometry *g = NULL;

	switch (this->type) {
	case POINTSET: {
		PolySet *p = new PolySet(3);
		g = p;
		p->setConvexity(this->convexity);
        // Inspired by http://nuklei.sourceforge.net/doxygen/KernelCollectionMesh_8cpp_source.html
        std::list<PointVectorPairK> points;
        if( this->points->type() == ValuePtr::undefined ) {
            PRINT("Usage pointset():");
            PRINT("  pointset( points,neighbors,scale_num_points,sharpness_angle,edge_sensitivity,neighbor_radius,angle,radius,distance,convexity ) ");
            PRINT("    where: ");
            PRINT("        points : list_of_3D_points || [ list_of_3D_points, list_of_3D_normals ] ");
            PRINT("        neighbors : jet_estimate_normals & mst_orient_normals 'neighbors' parameter ");
            PRINT("        scale_num_points : >1 enables edge_aware_upsample_point_set() ");
            PRINT("        sharpness_angle,edge_sensitivity,neighbor_radius : edge_aware_upsample_point_set() parameters ");
            PRINT("        angle,radius,distance : Surface_mesh_default_criteria parameters ");
            return p;
        }
        size_t num_points=this->points->toVector().size();
        ValuePtr point_vec,norm_vec;
        if(num_points==2) {
            point_vec=this->points[0];
            norm_vec=this->points[1];
            if( point_vec->toVector().size() == norm_vec->toVector().size() ) num_points=point_vec->toVector().size();
        } else {
            point_vec=this->points;
            norm_vec=ValuePtr::undefined;
        }

        int nb_neighbors = (int)this->neighbors;
        PRINTB("POINTSET nb_neighbors: %d",nb_neighbors);
        if( num_points < 4 && num_points > 0 ) {
            PRINTB("ERROR: Only %d points given", num_points);
            return p;
        }
        if(0) if( nb_neighbors < num_points ) {
            PRINTB("POINTSET neighbors < num_points; setting neighbors = num_points = %d",num_points);
            nb_neighbors=(int)num_points;
        }
        for (size_t i=0; i<num_points; i++)
        {
            double px, py, pz;
            if (!point_vec->toVector()[i]->getVec3(px,py,pz) ||
                    isinf(px) || isinf(py) || isinf(pz)) {
                PRINTB("ERROR: Unable to convert point at index %d to a vec3 of numbers", i);
                return p;
            }
            PointK ptk(px,py,pz);
            double px2, py2, pz2;
            if ( norm_vec==ValuePtr::undefined || !norm_vec->toVector()[i]->getVec3(px2,py2,pz2) ||
                    isinf(px2) || isinf(py2) || isinf(pz2)) {
                points.push_back(std::make_pair(ptk, VectorK()));
            } else {
                VectorK veck(px2,py2,pz2);
                points.push_back(std::make_pair(ptk, veck));
            }
        }
        if ( nb_neighbors > 3 ) {
	        PRINT("POINTSET: Running jet_estimate_normals...");
	        CGAL::jet_estimate_normals(points.begin(), points.end(),
	                CGAL::First_of_pair_property_map<PointVectorPairK>(),
	                CGAL::Second_of_pair_property_map<PointVectorPairK>(),
	                nb_neighbors);
	        PRINT("POINTSET: Running mst_orient_normals...");
	        std::list<PointVectorPairK>::iterator unoriented_points_begin =
	            CGAL::mst_orient_normals(points.begin(), points.end(),
	                CGAL::First_of_pair_property_map<PointVectorPairK>(),
	                CGAL::Second_of_pair_property_map<PointVectorPairK>(),
	                nb_neighbors);
	        PRINT("POINTSET: Running points.erase...");
	        points.erase(unoriented_points_begin, points.end());
        }
        PRINT("POINTSET: Running swap(points)...");
        std::list<PointVectorPairK>(points).swap(points);
        PRINT("POINTSET: Running edge_aware_upsample_point_set...");
        const double scale_num_points = this->scale_num_points; // 2.0;
        if ( scale_num_points > 1.0 ) {
	        const double sharpness_angle = this->sharpness_angle; // 25;
	        const double edge_sensitivity = this->edge_sensitivity; // 0;
	        const double neighbor_radius = this->neighbor_radius; // 0.25;
	        const unsigned int number_of_output_points = (unsigned int) (num_points * scale_num_points);
	        CGAL::edge_aware_upsample_point_set(
	                points.begin(), points.end(), std::back_inserter(points),
	                CGAL::First_of_pair_property_map<PointVectorPairK>(),
	                CGAL::Second_of_pair_property_map<PointVectorPairK>(),
	                sharpness_angle,
	                edge_sensitivity,
	                neighbor_radius,
	                number_of_output_points);
        }
        FTK sm_angle = this->angle; // 20.0;
        PRINTB("POINTSET sm_angle: %d",sm_angle);
        FTK sm_radius = this->radius; // 30;
        PRINTB("POINTSET sm_radius: %d",sm_radius);
        FTK sm_distance = this->distance; // 0.05;
        PRINTB("POINTSET sm_distance: %d",sm_distance);
        PointListK pl;
        for (std::list<PointVectorPairK>::const_iterator i = points.begin(); i != points.end(); ++i)
        {
            pl.push_back(Point_with_normalK(i->first, i->second));
        }
        PRINT("POINTSET: Running Poisson_reconstruction_functionK...");
        Poisson_reconstruction_functionK function(
                pl.begin(), pl.end(),
                CGAL::make_normal_of_point_with_normal_pmap(PointListK::value_type())
                );
        if ( ! function.compute_implicit_function() ) {
            PRINT("ERROR: Poisson reconstruction function error.");
            return p;
        }
        PRINT("POINTSET: Running compute_average_spacing...");
        FTK average_spacing = CGAL::compute_average_spacing(pl.begin(), pl.end(),
                6
                );
        PRINTB("POINTSET:  average_spacing = %d",average_spacing);
        PointK inner_point = function.get_inner_point();
        SphereK bsphere = function.bounding_sphere();
        FTK radius = std::sqrt(bsphere.squared_radius());
        PRINTB("POINTSET:  bsphere radius = %d",radius);
        FTK sm_sphere_radius = 5.0 * radius;
        FTK sm_dichotomy_error = sm_distance*average_spacing/1000.0;
        Surface_3K surface(function,
                SphereK(inner_point,sm_sphere_radius*sm_sphere_radius),
                sm_dichotomy_error/sm_sphere_radius);
        CGAL::Surface_mesh_default_criteria_3<STr> criteria(sm_angle
                , sm_radius*average_spacing
                , sm_distance*average_spacing);
        STr tr;
        C2t3 c2t3(tr);
        PRINT("POINTSET: Running make_surface_mesh...");
        CGAL::make_surface_mesh(c2t3
                , surface
                , criteria
                // , CGAL::Manifold_with_boundary_tag());
                , CGAL::Manifold_tag());
        if(tr.number_of_vertices() == 0 )
        {
            PRINT("ERROR: make_surface_mesh failure");
            return p;
        }
        PolyhedronK output_mesh;
        PRINT("POINTSET: Running output_surface_facets_to_polyhedron...");
        CGAL::output_surface_facets_to_polyhedron(c2t3, output_mesh);
        // createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
        bool err = CGALUtils::createPolySetFromPolyhedron(output_mesh, *p);
        if( err ) {
            PRINT("ERROR: createPolySetFromPolyhedron failure");
        }
	}
	}

	return g;
}

std::string PrimitiveCGALNode::toString() const
{
	std::stringstream stream;

	stream << this->name();

	switch (this->type) {
	case POINTSET:
		stream << "(points = " << *this->points 
                    // jet_estimate_normals & mst_orient_normals 'nb_neighbors' parameter
                    << ", neighbors = " << this->neighbors 
                    // edge_aware_upsample_point_set parameters
                    << ", scale_num_points = " << this->scale_num_points
                    << ", sharpness_angle = " << this->sharpness_angle
                    << ", edge_sensitivity = " << this->edge_sensitivity
                    << ", neighbor_radius = " << this->neighbor_radius
                    // Surface_mesh_default_criteria_3 parameters
                    << ", angle = " << this->angle 
                    << ", radius = " << this->radius 
                    << ", distance = " << this->distance 
                    // convexity
					<< ", convexity = " << this->convexity 
                    << ")";
			break;
	default:
		assert(false);
	}

	return stream.str();
}

void register_builtin_cgal_primitives()
{
	Builtins::init("pointset", new PrimitiveCGALModule(POINTSET));
}

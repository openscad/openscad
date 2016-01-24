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
    POINTSET,
    SKIN_SURFACE
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
		case SKIN_SURFACE:
			return "skin_surface";
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
    bool verbose;
	ValuePtr points, paths, faces;
	virtual Geometry *createGeometry() const;
    int jen_k, mon_k;
    double eaup_scale_num_points;
    unsigned int eaup_number_of_output_points;
    double eaup_sharpness_angle, eaup_edge_sensitivity, eaup_neighbor_radius;
    int cas_k;
    double smdc_angle, smdc_radius, smdc_distance;
    ValuePtr weights;
    double shrink_factor;
    int subdivisions;
    bool grow_balls;
    double weight;
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
    node->verbose = false;

	AssignmentList args;

	switch (this->type) {
	case POINTSET:
		args += Assignment("points")
            // jet_estimate_normals (jen)
            , Assignment("jen_k")
            // mst_orient_normals (mon)
            , Assignment("mon_k")
            // edge_aware_upsample_point_set (eaup)
            , Assignment("eaup_scale_num_points") // number_of_output_points = eaup_scale_num_points * len(points)
            , Assignment("eaup_number_of_output_points") // Override derived number_of_output_points calculation
            , Assignment("eaup_sharpness_angle")
            , Assignment("eaup_edge_sensitivity")
            , Assignment("eaup_neighbor_radius")
            // compute_average_spacing (cas)
            , Assignment("cas_k")
            // Surface_mesh_default_criteria (smdc)
            , Assignment("smdc_angle"), Assignment("smdc_radius"), Assignment("smdc_distance")
            , Assignment("convexity"), Assignment("verbose");
		break;
	case SKIN_SURFACE:
		args += Assignment("points")
            , Assignment("weights")
            , Assignment("shrink_factor")
            , Assignment("subdivisions")
            , Assignment("grow_balls")
            , Assignment("weight")
            , Assignment("convexity"), Assignment("verbose");
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
        bool verbose = c.lookup_variable("verbose")->toBool();
        // jet_estimate_normals 'neighbors' parameter
        ValuePtr jen_k = c.lookup_variable("jen_k");
        if (jen_k->type() == Value::NUMBER) {
            node->jen_k = jen_k->toDouble();
        } else {
            node->jen_k = 16;
        }
        if (node->jen_k < 0) node->jen_k = 0;
        if (node->jen_k > 0 && node->jen_k < 5 ) node->jen_k = 5;
        if (verbose) PRINTB("POINTSET jen_k: %d",node->jen_k);

        // mst_orient_normals 'neighbors' parameter
        ValuePtr mon_k = c.lookup_variable("mon_k");
        if (mon_k->type() == Value::NUMBER) {
            node->mon_k = mon_k->toDouble();
        } else {
            node->mon_k = 16;
        }
        if ( node->mon_k < 6 ) node->mon_k = 16;
        if (verbose) PRINTB("POINTSET mon_k: %d",node->mon_k);

        // edge_aware_upsample_point_set parameters

        ValuePtr eaup_scale_num_points = c.lookup_variable("eaup_scale_num_points");
        if ( eaup_scale_num_points->type() == Value::NUMBER) {
            const double snp = eaup_scale_num_points->toDouble();
            if( snp > 1.0 ) {
                node->eaup_scale_num_points = snp;
            } else {
                node->eaup_scale_num_points = 0;
            }
        } else {
            node->eaup_scale_num_points = 0;
        }
        if (verbose) PRINTB("POINTSET eaup_scale_num_points: %d",node->eaup_scale_num_points);

        ValuePtr eaup_number_of_output_points = c.lookup_variable("eaup_number_of_output_points");
        if ( eaup_number_of_output_points->type() == Value::NUMBER) {
            const double enoop = eaup_number_of_output_points->toDouble();
            if( enoop > 3 ) {
                node->eaup_number_of_output_points = enoop;
            } else {
                node->eaup_number_of_output_points = 0;
            }
        } else {
            node->eaup_number_of_output_points = 0;
        }
        if (verbose) PRINTB("POINTSET eaup_number_of_output_points: %d",node->eaup_number_of_output_points);

        ValuePtr eaup_sharpness_angle = c.lookup_variable("eaup_sharpness_angle");
        if ( eaup_sharpness_angle->type() == Value::NUMBER) {
            const double sat = eaup_sharpness_angle->toDouble();
            node->eaup_sharpness_angle = sat;
        } else {
            node->eaup_sharpness_angle = 25.0;
        }
        if (verbose) PRINTB("POINTSET eaup_sharpness_angle: %d",node->eaup_sharpness_angle);

        ValuePtr eaup_edge_sensitivity = c.lookup_variable("eaup_edge_sensitivity");
        if ( eaup_edge_sensitivity->type() == Value::NUMBER) {
            const double es = eaup_edge_sensitivity->toDouble();
            if( es < 0.0 ) {
                node->eaup_edge_sensitivity = 0.0;
            } else if( es > 1.0 ) {
                node->eaup_edge_sensitivity = 1.0;
            } else {
                node->eaup_edge_sensitivity = es;
            }
        } else {
            node->eaup_edge_sensitivity = 0.0;
        }
        if (verbose) PRINTB("POINTSET eaup_edge_sensitivity: %d",node->eaup_edge_sensitivity);

        // neighbor_radius = 0.25
        ValuePtr eaup_neighbor_radius = c.lookup_variable("eaup_neighbor_radius");
        if ( eaup_neighbor_radius->type() == Value::NUMBER) {
            const double nr = eaup_neighbor_radius->toDouble();
            if( nr > 0 ) {
                node->eaup_neighbor_radius = nr;
            } else {
                node->eaup_neighbor_radius = 0.25;
            }
        } else {
            node->eaup_neighbor_radius = 0.25;
        }
        if (verbose) PRINTB("POINTSET eaup_neighbor_radius: %d",node->eaup_neighbor_radius);

        // compute_average_spacing
        ValuePtr cas_k = c.lookup_variable("cas_k");
        if ( cas_k->type() == Value::NUMBER) {
            const double k = cas_k->toDouble();
            if( k > 0 ) {
                node->cas_k = k;
            } else {
                node->cas_k = 6;
            }
        } else {
            node->cas_k = 6;
        }
        if (verbose) PRINTB("POINTSET cas_k: %d",node->cas_k);

        // Surface_mesh_default_criteria_3 parameters
        ValuePtr smdc_angle = c.lookup_variable("smdc_angle");
        if (smdc_angle->type() == Value::NUMBER) {
            node->smdc_angle = smdc_angle->toDouble();
        } else {
            node->smdc_angle = 20.0;
        }
        if (verbose) PRINTB("POINTSET smdc_angle: %d",node->smdc_angle);

        ValuePtr smdc_radius = c.lookup_variable("smdc_radius");
        if (smdc_radius->type() == Value::NUMBER) {
            node->smdc_radius = smdc_radius->toDouble();
        } else {
            node->smdc_radius = 30;
        }
        if (verbose) PRINTB("POINTSET smdc_radius: %d",node->smdc_radius);

        ValuePtr smdc_distance = c.lookup_variable("smdc_distance");
        if (smdc_distance->type() == Value::NUMBER) {
            node->smdc_distance = smdc_distance->toDouble();
        } else {
            node->smdc_distance = 0.05;
        }
        if (verbose) PRINTB("POINTSET smdc_distance: %d",node->smdc_distance);

		break;
	}
	case SKIN_SURFACE: {
        bool verbose = c.lookup_variable("verbose")->toBool();
		node->points = c.lookup_variable("points");
		node->weights = c.lookup_variable("weights");

        ValuePtr shrink_factor = c.lookup_variable("shrink_factor");
        if (shrink_factor->type() == Value::NUMBER) {
            node->shrink_factor = shrink_factor->toDouble();
        } else {
            node->shrink_factor = 0.5;
        }
        if (verbose) PRINTB("SKIN_SURFACE shrink_factor: %d",node->shrink_factor);

        ValuePtr subdivisions = c.lookup_variable("subdivisions");
        if (subdivisions->type() == Value::NUMBER) {
            node->subdivisions = subdivisions->toDouble();
        } else {
            node->subdivisions = 0;
        }
        if (verbose) PRINTB("SKIN_SURFACE subdivisions: %d",node->subdivisions);

        ValuePtr grow_balls = c.lookup_variable("grow_balls");
        if (grow_balls->type() == Value::BOOL) {
            node->grow_balls = grow_balls->toBool();
        } else {
            node->grow_balls = true;
        }
        if (verbose) PRINTB("SKIN_SURFACE grow_balls: %d",node->grow_balls);

        ValuePtr weight = c.lookup_variable("weight");
        if ( weight->type() == Value::NUMBER) {
            node->weight = weight->toDouble();
        } else {
            node->weight = 1.25;
        }

		break;
	}
	}

	node->convexity = c.lookup_variable("convexity", true)->toDouble();
	if (node->convexity < 1)
		node->convexity = 1;


	return node;
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
            PRINT("  pointset( points ) ");
            PRINT("        points : list_of_3D_points || [ list_of_3D_points, list_of_3D_normals ] ");
            PRINT("    Optional parameters: ");
            PRINT("        jen_k : jet_estimate_normals 'neighbors' parameter - http://doc.cgal.org/latest/Point_set_processing_3/index.html#Point_set_processing_3NormalEstimation ");
            PRINT("        mon_k : mst_orient_normals 'neighbors' parameter - http://doc.cgal.org/latest/Point_set_processing_3/index.html#Point_set_processing_3NormalOrientation ");
            PRINT("        eaup_scale_num_points : >1 enables edge_aware_upsample_point_set() - http://doc.cgal.org/latest/Point_set_processing_3/index.html#Point_set_processing_3Upsampling ");
            PRINT("        eaup_number_of_output_points : >len(points) ");
            PRINT("        eaup_{sharpness_angle,edge_sensitivity,neighbor_radius} : edge_aware_upsample_point_set() parameters ");
            PRINT("        cas_k : compute_average_spacing 'neighbors' parameter - http://doc.cgal.org/latest/Point_set_processing_3/index.html#Point_set_processing_3Analysis ");
            PRINT("        smdc_angle,smdc_radius,smdc_distance : Surface_mesh_default_criteria parameters - http://doc.cgal.org/latest/Surface_mesher/index.html#SurfaceMesher_section_interface ");
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

        if( num_points < 4 && num_points > 0 ) {
            PRINTB("ERROR: Only %d points given", num_points);
            return p;
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

        int nb_jen_k = (int)this->jen_k;
        if (verbose) PRINTB("POINTSET jen_k: %d",nb_jen_k);
        if ( nb_jen_k > 3 ) {
	        if (verbose) PRINT("POINTSET: Running jet_estimate_normals...");
	        CGAL::jet_estimate_normals(points.begin(), points.end(),
	                CGAL::First_of_pair_property_map<PointVectorPairK>(),
	                CGAL::Second_of_pair_property_map<PointVectorPairK>(),
	                nb_jen_k);
        } else {
            if (verbose) PRINT("POINTSET: jet_estimate_normals disabled.");
        }

        int nb_mon_k = (int)this->mon_k;
        if (verbose) PRINTB("POINTSET mon_k: %d",nb_mon_k);
        if ( nb_mon_k > 3) {
	        if (verbose) PRINT("POINTSET: Running mst_orient_normals...");
	        std::list<PointVectorPairK>::iterator unoriented_points_begin =
	            CGAL::mst_orient_normals(points.begin(), points.end(),
	                CGAL::First_of_pair_property_map<PointVectorPairK>(),
	                CGAL::Second_of_pair_property_map<PointVectorPairK>(),
	                nb_mon_k);
	        if (verbose) PRINT("POINTSET: Running points.erase...");
	        points.erase(unoriented_points_begin, points.end());
        } else {
            if (verbose) PRINT("POINTSET: mst_orient_normals disabled.");
        }

        if (verbose) PRINT("POINTSET: Running swap(points)...");
        std::list<PointVectorPairK>(points).swap(points);

        const double scale_num_points = this->eaup_scale_num_points; // 2.0;
        unsigned int number_of_output_points = (unsigned int ) this->eaup_number_of_output_points;
        if ( scale_num_points > 1.0 || number_of_output_points > num_points ) {
            if (verbose) PRINT("POINTSET: Running edge_aware_upsample_point_set...");
	        if (scale_num_points > 1.0) number_of_output_points = (unsigned int) (num_points * scale_num_points);
	        const double sharpness_angle = this->eaup_sharpness_angle; // 25;
	        const double edge_sensitivity = this->eaup_edge_sensitivity; // 0;
	        const double neighbor_radius = this->eaup_neighbor_radius; // 0.25;
	        CGAL::edge_aware_upsample_point_set(
	                points.begin(), points.end(), std::back_inserter(points),
	                CGAL::First_of_pair_property_map<PointVectorPairK>(),
	                CGAL::Second_of_pair_property_map<PointVectorPairK>(),
	                eaup_sharpness_angle,
	                eaup_edge_sensitivity,
	                eaup_neighbor_radius,
	                number_of_output_points);
        } else {
            if (verbose) PRINT("POINTSET: edge_aware_upsample_point_set disabled.");
        }
        PointListK pl;
        for (std::list<PointVectorPairK>::const_iterator i = points.begin(); i != points.end(); ++i)
        {
            pl.push_back(Point_with_normalK(i->first, i->second));
        }
        if (verbose) PRINT("POINTSET: Running Poisson_reconstruction_functionK...");
        Poisson_reconstruction_functionK function(
                pl.begin(), pl.end(),
                CGAL::make_normal_of_point_with_normal_pmap(PointListK::value_type())
                );
        if ( ! function.compute_implicit_function() ) {
            PRINT("ERROR: Poisson reconstruction function error.");
            return p;
        }

        if (verbose) PRINT("POINTSET: Running compute_average_spacing...");
        unsigned int cas_k = (unsigned int) this->cas_k;
        if (verbose) PRINTB("POINTSET: cas_k = %d",cas_k);
        FTK average_spacing = CGAL::compute_average_spacing(pl.begin(), pl.end(),
               cas_k 
                );
        if (verbose) PRINTB("POINTSET:  average_spacing = %d",average_spacing);
        PointK inner_point = function.get_inner_point();
        SphereK bsphere = function.bounding_sphere();
        FTK radius = std::sqrt(bsphere.squared_radius());
        if (verbose) PRINTB("POINTSET:  bsphere radius = %d",radius);
        FTK sm_sphere_radius = 5.0 * radius;

        FTK smdc_angle = this->smdc_angle; // 20.0;
        if (verbose) PRINTB("POINTSET smdc_angle: %d",smdc_angle);
        FTK smdc_radius = this->smdc_radius; // 30;
        if (verbose) PRINTB("POINTSET smdc_radius: %d",smdc_radius);
        FTK smdc_distance = this->smdc_distance; // 0.05;
        if (verbose) PRINTB("POINTSET smdc_distance: %d",smdc_distance);

        FTK sm_dichotomy_error = smdc_distance*average_spacing/1000.0;
        Surface_3K surface(function,
                SphereK(inner_point,sm_sphere_radius*sm_sphere_radius),
                sm_dichotomy_error/sm_sphere_radius);
        CGAL::Surface_mesh_default_criteria_3<STr> criteria(smdc_angle
                , smdc_radius*average_spacing
                , smdc_distance*average_spacing);
        STr tr;
        C2t3 c2t3(tr);
        if (verbose) PRINT("POINTSET: Running make_surface_mesh...");
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
        if (verbose) PRINT("POINTSET: Running output_surface_facets_to_polyhedron...");
        CGAL::output_surface_facets_to_polyhedron(c2t3, output_mesh);
        // createPolySetFromPolyhedron(const Polyhedron &p, PolySet &ps);
        bool err = CGALUtils::createPolySetFromPolyhedron(output_mesh, *p);
        if( err ) {
            PRINT("ERROR: createPolySetFromPolyhedron failure");
        }
        break;
	}
	case SKIN_SURFACE: {
		PolySet *p = new PolySet(3);
		g = p;
		p->setConvexity(this->convexity);
        std::list<WeightedK> points;
        size_t num_points=this->points->toVector().size();
        ValuePtr point_vec=this->points;
        ValuePtr weight_vec=this->weights;
        double weight;
        if(num_points==2) {
            point_vec=this->points[0];
            weight_vec=this->points[1];
            if( point_vec->toVector().size() <= weight_vec->toVector().size() ) {
                num_points=point_vec->toVector().size();
            } else {
                PRINT("ERROR: Not enough weights for points.");
                return p;
            }
        } else {
            weight=this->weight;
        }
        for (size_t i=0; i<num_points;i++)
        {
            double px, py, pz;
            if (!point_vec->toVector()[i]->getVec3(px,py,pz) ||
                    isinf(px) || isinf(py) || isinf(pz)) {
                PRINTB("ERROR: Unable to convert point and index %d to a vec3 of numbers", i);
                return p;
            }
            PointK ptk(px,py,pz);
            double wt;
            if ( weight_vec==ValuePtr::undefined || weight_vec->toVector()[i]->type()!=Value::NUMBER ) {
                points.push_front(WeightedK(ptk,weight));
            } else {
                wt = weight_vec->toVector()[i]->toDouble();
                points.push_front(WeightedK(ptk,wt));
            }
        }
        PolyhedronK output_mesh;
        double shrink_factor = this->shrink_factor;
        int subdivisions = (int) this->subdivisions;
        bool grow_balls = this->grow_balls;
        CGAL::make_skin_surface_mesh_3(output_mesh, points.begin(), points.end(), shrink_factor, subdivisions, grow_balls);
        bool err = CGALUtils::createPolySetFromPolyhedron(output_mesh, *p);
        if( err ) {
            PRINT("ERROR: createPolySetFromPolyhedron failure");
        }
        break;
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
		stream << "(points = " << this->points 
                    // jet_estimate_normals 
                    << ", jen_k = " << this->jen_k 
                    // mst_orient_normals 
                    << ", mon_k = " << this->mon_k 
                    // edge_aware_upsample_point_set parameters
                    << ", eaup_scale_num_points = " << this->eaup_scale_num_points
                    << ", eaup_number_of_output_points = " << this->eaup_number_of_output_points
                    << ", eaup_sharpness_angle = " << this->eaup_sharpness_angle
                    << ", eaup_edge_sensitivity = " << this->eaup_edge_sensitivity
                    << ", eaup_neighbor_radius = " << this->eaup_neighbor_radius
                    // compute_average_spacing
                    << ", cas_k = " << this->cas_k
                    // Surface_mesh_default_criteria_3 parameters
                    << ", smdc_angle = " << this->smdc_angle 
                    << ", smdc_radius = " << this->smdc_radius 
                    << ", smdc_distance = " << this->smdc_distance 
                    // convexity
					<< ", convexity = " << this->convexity 
					<< ", verbose = " << this->verbose 
                    << ")";
			break;
	case SKIN_SURFACE:
		stream << "(points = " << this->points 
                    << ", weights = " << this->weights
                    << ", shrink_factor = " << this->shrink_factor
                    << ", subdivisions = " << this->subdivisions
                    << ", grow_balls = " << this->grow_balls
					<< ", convexity = " << this->convexity 
					<< ", verbose = " << this->verbose 
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
	Builtins::init("skin_surface", new PrimitiveCGALModule(SKIN_SURFACE));
}

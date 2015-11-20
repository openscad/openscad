//
//  sweepnode.cpp
//
//
//  Created by Oskar Linde on 2014-05-17.
//
//

#include "sweepnode.h"
#include "module.h"
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "polyset.h"
#include "visitor.h"

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class SweepModule : public AbstractModule
{
public:
	SweepModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
};

double deg_to_rad(double deg) {
	return deg / 180 * M_PI;
}

/* The interpretation of the transforms are:
 *
 * - a single number is interpreted as a angle for a rotation around the z-axis
 * - a vector of two numbers is interpreted as a 2-d translation
 * - a vector of three numbers is interpreted as a 3-d translation
 * - a vector of vectors is interpreted as a transformation matrix. Valid sizes are:
 *    2x3 Interpreted as a 2-d Affine transformation
 *    3x3 Interpreted as a 2-d Projective transformation
 *    3x4 Interpreted as a 3-d Affine transformation
 *    4x4 Interpreted as a 3-d Projective transformation
 */

Eigen::Projective3d make_transform(Value const& v) {
	Eigen::Projective3d ret = Eigen::Projective3d::Identity();

	if (v.type() == Value::NUMBER) {
		ret.matrix().topLeftCorner<2,2>() = Eigen::Rotation2D<double>(deg_to_rad(v.toDouble())).toRotationMatrix();
		return ret;
	}

	if (v.type() != Value::VECTOR || v.toVector().empty()) {
		PRINT("Invalid transformation value to sweep()");
		return ret;
	}

	std::vector<Value> const& vec = v.toVector();

	if (vec[0].type() == Value::NUMBER) {

		if (vec.size() == 2) {
			ret.matrix()(0,2) = vec[0].toDouble();
			ret.matrix()(1,2) = vec[1].toDouble();
		} else if (vec.size() == 3) {
			ret.matrix()(0,2) = vec[0].toDouble();
			ret.matrix()(1,2) = vec[1].toDouble();
			ret.matrix()(2,2) = vec[2].toDouble();
		} else {
			PRINT("Invalid transformation vector to sweep()");
			return ret;
		}

		return ret;
	}

	if (vec[0].type() == Value::VECTOR) {
		int M = vec.size();
		int N = vec[0].toVector().size();

		if (!(M == 2 && N == 3 ||
			  M == 3 && N == 3 ||
			  M == 3 && N == 4 ||
			  M == 4 && N == 4)) {
			PRINT("Invalid transformation matrix size to sweep()");
			return ret;
		}

		// Make sure we have a proper matrix
		for (int i = 1; i < M; i++) {
			if (vec[i].toVector().size() != N) {
				PRINT("Not a valid matrix as argument to sweep()");
				return ret;
			}
		}

		if (N == 3) {
			for (int i = 0; i < M; i++) {
				int r = i == 2 ? 3 : i;
				ret.matrix()(r,0) = vec[i].toVector()[0].toDouble();
				ret.matrix()(r,1) = vec[i].toVector()[1].toDouble();
				ret.matrix()(r,3) = vec[i].toVector()[2].toDouble();
			}
		} else if (N == 4) {
			for (int i = 0; i < M; i++) {
				for (int j = 0; j < N; j++) {
					ret.matrix()(i,j) = vec[i].toVector()[j].toDouble();
				}
			}
		} else assert(0);

		return ret;
	}

	PRINT("Invalid argument to sweep()");
	return ret;
}


AbstractNode *SweepModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	SweepNode *node = new SweepNode(inst);

	AssignmentList args;
	args += Assignment("path");

	Context c(ctx);
	c.setVariables(args, evalctx);

	Value path_value = c.lookup_variable("path");

	if (path_value.type() != Value::VECTOR || path_value.toVector().empty() ) {
		PRINT("Error: sweep(): invalid path");
	}

	std::vector<Value> const& path = path_value.toVector();
	node->sweep_path.resize(path.size());

	int dimensionality = 2;

	for (int p = 0; p < path.size(); p++) {
		node->sweep_path[p] = make_transform(path[p]);

		if (dimensionality == 2) {
			int i = 2;
			for (int j = 0; j < 4; j++) {
				if (node->sweep_path[p](i,j) != (i==j?1:0)) {
					dimensionality = 3;
				}
			}
		}
	}

	node->dimensions = dimensionality;

	node->convexity = c.lookup_variable("convexity", true).toDouble();

	if (node->convexity <= 0)
		node->convexity = 1;

	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}


std::string SweepNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "(";
	stream <<
	"convexity = " << this->convexity << ", "
	"path = [";

	for (int k = 0; k < this->sweep_path.size(); k++) {
		stream << "[";
		for (int i = 0; i < 4; i++) {
			stream << "[";
			for (int j = 0; j < 4; j++) {
				stream << this->sweep_path[k].matrix()(i,j) << ",";
			}
			stream << "],";
		}
		stream << "],";
	}

	stream << "])";

	return stream.str();
}


void register_builtin_sweep()
{
	Builtins::init("sweep", new SweepModule());
}

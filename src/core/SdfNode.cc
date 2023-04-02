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
#include "ModuleInstantiation.h"
#include "node.h"
#include "PolySet.h"
#include "Builtins.h"
#include "BuiltinContext.h"
#include "Children.h"
#include "Expression.h"
#include "FunctionType.h"
#include "Parameters.h"
#include "printutils.h"
#include "fileutils.h"
#include "handle_dep.h"
#include "ext/lodepng/lodepng.h"

#include <cstdint>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <libfive/oracle/oracle_storage.hpp>
#include <libfive/oracle/oracle_clause.hpp>
#include <libfive/tree/tree.hpp>
#include <libfive/render/brep/mesh.hpp>
#include <libfive/render/brep/settings.hpp>
#include <libfive/render/brep/region.hpp>

class SdfNode : public LeafNode
{
public:
  VISITABLE();
  SdfNode(const ModuleInstantiation *mi) : LeafNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "sdf"; }

  Parameters *parameters;
  const Geometry *createGeometry() const override;
private:
};

static std::shared_ptr<AbstractNode> builtin_sdf(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  auto node = std::make_shared<SdfNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"sdf", "bounding_box", "resolution"}, {});
  node->parameters = &parameters;

  return node;
}

// 
// Thanks Doug! All this code is a modification from libfive_mesher from the Curv project.
// 
// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0
//
struct OpenSCADOracle : public libfive::OracleStorage<LIBFIVE_EVAL_ARRAY_SIZE>
{
    Parameters *parameters_;
    OpenSCADOracle(Parameters *parameters) : parameters_(parameters) {}

    void evalInterval(libfive::Interval& out) override
    {
        // Since Curv (and OpenSCAD) currently doesn't do interval arithmetic,
        // return a large interval. Which magic number do I choose?
        //
        // mkeeter: If the top-level tree is just this CurvOracle, then
        // returning [-inf, inf] should be fine; however, if you're
        // transforming it further, then I agree that the math could get iffy.
        // You could also return [NaN, NaN], which should cause interval
        // subdivision to always subdivide (down to individual voxels).

        // This gives rounded edges for a large cube (`cube 9999999`).
        //out = {-10000.0, 10000.0};

        // This gives the same results (rounded edges) for a large cube.
        // I'm using infinity because it is "the least magic" alternative
        // and is not overly scale dependent like 10000 is.
        out = { -std::numeric_limits<double>::infinity(),
                 std::numeric_limits<double>::infinity() };

      #if 0
        // This gives the same results (rounded edges) for a large cube.
        out = { std::numeric_limits<double>::quiet_NaN(),
                std::numeric_limits<double>::quiet_NaN() };
      #endif
    }
    void evalPoint(float& out, size_t index=0) override
    {
        const auto pt = points.col(index);
        const auto &sdf = parameters_->get("sdf").toFunction();
        auto expr = sdf.getExpr().get();
        AssignmentList al = AssignmentList();
        al.push_back(std::shared_ptr<Assignment>(new Assignment("x", std::shared_ptr<Expression>(new Literal(pt.x())))));
        al.push_back(std::shared_ptr<Assignment>(new Assignment("y", std::shared_ptr<Expression>(new Literal(pt.y())))));
        al.push_back(std::shared_ptr<Assignment>(new Assignment("z", std::shared_ptr<Expression>(new Literal(pt.z())))));
        auto fc = FunctionCall(expr, al, Location::NONE);
        auto val = fc.evaluate(sdf.getContext());
        out = val.toDouble();
    }
    void checkAmbiguous(Eigen::Block<
        Eigen::Array<bool, 1, LIBFIVE_EVAL_ARRAY_SIZE>,
        1, Eigen::Dynamic>) override
    {
        // Nothing to do here, because we can only find one derivative
        // per point. Points on sharp features may not be handled correctly.
    }
    void evalFeatures(boost::container::small_vector<libfive::Feature, 4>& out)
    override {
        // Find one derivative with partial differences.

        auto resolution = parameters_->get("resolution").toDouble();

        float centre, dx, dy, dz;
        Eigen::Vector3f before = points.col(0);
        evalPoint(centre);

        points.col(0) = before + Eigen::Vector3f(resolution, 0.0, 0.0);
        evalPoint(dx);

        points.col(0) = before + Eigen::Vector3f(0.0, resolution, 0.0);
        evalPoint(dy);

        points.col(0) = before + Eigen::Vector3f(0.0, 0.0, resolution);
        evalPoint(dz);

        points.col(0) = before;

        out.push_back(
            Eigen::Vector3f(dx - centre, dy - centre, dz - centre)
            .normalized());
    }
};

struct OpenSCADOracleClause : public libfive::OracleClause
{
    Parameters *parameters_;
    OpenSCADOracleClause(Parameters *parameters) : parameters_(parameters)
    {}
    std::unique_ptr<libfive::Oracle> getOracle() const override
    {
        return std::unique_ptr<libfive::Oracle>(new OpenSCADOracle(parameters_));
    }
    std::string name() const override { return "OpenSCADOracleClause"; }
};


const Geometry *SdfNode::createGeometry() const
{
  double resolution = this->parameters->get("resolution").toDouble();

  const auto &bounding_box_tmp = this->parameters->get("bounding_box").toVector();
  std::vector<double> bounding_box;
  for(const auto &val : bounding_box_tmp) {
    bounding_box.push_back(val.toDouble());
  }

  double x1 = bounding_box[0];
  double y1 = bounding_box[1];
  double z1 = bounding_box[2];
  double x2 = bounding_box[3];
  double y2 = bounding_box[4];
  double z2 = bounding_box[5];
  libfive::Region<3> region({x1, y1, z1}, {x2, y2, z2});

  libfive::Tree tree{std::make_unique<OpenSCADOracleClause>(this->parameters)};

  libfive::BRepSettings settings;
  settings.min_feature = resolution;
  settings.workers = std::thread::hardware_concurrency();
  settings.alg = libfive::DUAL_CONTOURING;
  std::unique_ptr<libfive::Mesh> mesh = libfive::Mesh::render(tree, region, settings);

  auto p = new PolySet(3);
  p->polygons.reserve(mesh->branes.size());
  for (auto t : mesh->branes) {
    p->append_poly();
    p->append_vertex(mesh->verts[t(0)].x(), mesh->verts[t(0)].y(), mesh->verts[t(0)].z());
    p->append_vertex(mesh->verts[t(1)].x(), mesh->verts[t(1)].y(), mesh->verts[t(1)].z());
    p->append_vertex(mesh->verts[t(2)].x(), mesh->verts[t(2)].y(), mesh->verts[t(2)].z());
  }

 return p;
}
std::string SdfNode::toString() const
{
  std::ostringstream stream;

  stream << this->name();

  return stream.str();
}

void register_builtin_sdf()
{
  Builtins::init("sdf", new BuiltinModule(builtin_sdf),
  {
    "sdf()",
  });
}

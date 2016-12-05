#include "CSGSubsetBuilder.h"
#include "state.h"
#include "csgops.h"
#include "transformnode.h"
#include "primitivenode.h"
#include "cgaladvnode.h"
#include "colornode.h"
#include "rendernode.h"
#include "printutils.h"
#include "GeometryEvaluator.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "GeometryUtils.h"
#include "Reindexer.h"
#include "hash.h"

#include <boost/range/adaptor/reversed.hpp>

/*!
	\class CSGSubsetBuilder

	A visitor responsible for creating a CSG tree out of a subset of OpenSCAD nodes.
*/

const AbstractNode *CSGSubsetBuilder::buildCSG(const AbstractNode &node)
{
	this->traverse(node);
	const AbstractNode *rootnode = this->transformed[&node];
	if (rootnode->getChildren().size() == 1) rootnode = rootnode->getChildren().front();
	return rootnode;
}

void CSGSubsetBuilder::addTransformedNode(const State &state, const AbstractNode *node)
{
	AbstractNode *newnode = this->transformed[node];
	if (state.isPostfix() && state.parent()) {
//	if (state.isPostfix() && state.parent() && newnode->getChildren().size() > 0) {
		if (dynamic_cast<CsgOpNode*>(newnode)) {
			if (newnode->getChildren().size() == 1) {
				this->transformed[state.parent()]->children.push_back(newnode->getChildren().front());
			}
			else if (newnode->getChildren().size() > 1) {
				this->transformed[state.parent()]->children.push_back(newnode); // FIXME: direct member access
			}
		}
		else {
			this->transformed[state.parent()]->children.push_back(newnode); // FIXME: direct member access
		}
	}
//	if (state.parent()) this->transformed[state.parent()]->children.push_back(newnode); // FIXME: direct member access
//	this->transformed[node] = newnode;
}

PrimitiveNode *CSGSubsetBuilder::evaluateGeometry(const State &state, const AbstractNode &node)
{
	shared_ptr<const Geometry> geom = this->geomevaluator->evaluateGeometry(node, false);
	if (geom && !geom->isEmpty()) {
		if (shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom)) {
			PolySet newps(*ps);
			newps.transform(state.matrix());
			
			IndexedPolygons mesh;
			GeometryUtils::createIndexedPolygonsFromPolySet(newps, mesh);
			
			Value::VectorType points;
			for (const auto &v : mesh.vertices) {
				Value::VectorType vec{v[0], v[1], v[2]};
				points.push_back(vec);
			}
			
			Value::VectorType faces;
			for (const auto &f : mesh.faces) {
				Value::VectorType face;
				for (const auto &i : boost::adaptors::reverse(f)) {
					face.push_back(i);
				}
				faces.push_back(face);
			}

			return new PolyhedronNode(node.modinst, points, faces);
		}
		else if (shared_ptr<const Polygon2d> poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
			Polygon2d newpoly(*poly);
			Transform2d mat2;
			mat2.matrix() << 
				state.matrix()(0,0), state.matrix()(0,1), state.matrix()(0,3),
				state.matrix()(1,0), state.matrix()(1,1), state.matrix()(1,3),
				state.matrix()(3,0), state.matrix()(3,1), state.matrix()(3,3);
			newpoly.transform(mat2);
			Reindexer<Vector2d> vertices;
			std::vector<IndexedFace> polygon;
			for (const auto &outline : newpoly.outlines()) {
				IndexedFace o;
				for (const auto &v : outline.vertices) o.push_back(vertices.lookup(v));
				polygon.push_back(o);
			}

			Value::VectorType points;
			const Vector2d *p = vertices.getArray();
			for (size_t i=0;i<vertices.size();i++) {
				Value::VectorType vec{p[i][0], p[i][1]};
				points.push_back(vec);
			}

			Value::VectorType paths;
			for (const auto &f : polygon) {
				Value::VectorType path;
				for (const auto &i : f) {
					path.push_back(i);
				}
				paths.push_back(path);
			}
			return new PolygonNode(node.modinst, points, paths);
		}
	}
	return nullptr;
}

Response CSGSubsetBuilder::visit(State &state, const AbstractNode &node)
{
	if (state.isPrefix()) {
		this->transformed[&node] = new CsgOpNode(node.modinst, OPENSCAD_UNION);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return ContinueTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const AbstractIntersectionNode &node)
{
	if (state.isPrefix()) {
		this->transformed[&node] = new CsgOpNode(node.modinst, OPENSCAD_INTERSECTION);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return ContinueTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const AbstractPolyNode &node)
{
	if (state.isPrefix()) {
		this->transformed[&node] = this->evaluateGeometry(state, node);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return PruneTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const CsgOpNode &node)
{
	if (state.isPrefix()) {
		this->transformed[&node] = new CsgOpNode(node.modinst, node.type);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return ContinueTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix()) {
		state.setMatrix(state.matrix() * node.matrix);
		this->transformed[&node] = new CsgOpNode(node.modinst, OPENSCAD_UNION);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return ContinueTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const ColorNode &node)
{
	if (state.isPrefix()) {
		// FIXME: Keep color?	if (!state.color().isValid()) state.setColor(node.color);
		this->transformed[&node] = new CsgOpNode(node.modinst, OPENSCAD_UNION);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return ContinueTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const RenderNode &node)
{
	if (state.isPrefix()) {
		this->transformed[&node] = this->evaluateGeometry(state, node);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	
	return PruneTraversal;
}

Response CSGSubsetBuilder::visit(State &state, const CgaladvNode &node)
{
	if (state.isPrefix()) {
		this->transformed[&node] = this->evaluateGeometry(state, node);
	}
	else if (state.isPostfix()) {
		this->addTransformedNode(state, &node);
	}
	return PruneTraversal;
}

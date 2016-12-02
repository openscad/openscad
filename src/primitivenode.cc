#include "primitivenode.h"
#include "polyset.h"
#include "Polygon2d.h"
#include "calc.h"
#include "printutils.h"

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
const Geometry *PrimitiveNode::createGeometry() const
{
	Geometry *g = NULL;

	switch (this->type) {
	case CUBE: {
		PolySet *p = new PolySet(3,true);
		g = p;
		if (this->x > 0 && this->y > 0 && this->z > 0 &&
			!std::isinf(this->x) > 0 && !std::isinf(this->y) > 0 && !std::isinf(this->z) > 0) {
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
	case SPHERE: {
		PolySet *p = new PolySet(3,true);
		g = p;
		if (this->r1 > 0 && !std::isinf(this->r1)) {
			struct ring_s {
				point2d *points;
				double z;
			};

			int fragments = Calc::get_fragments_from_r(r1, fn, fs, fa);
			int rings = (fragments+1)/2;
// Uncomment the following three lines to enable experimental sphere tesselation
//		if (rings % 2 == 0) rings++; // To ensure that the middle ring is at phi == 0 degrees

			ring_s *ring = new ring_s[rings];

//		double offset = 0.5 * ((fragments / 2) % 2);
			for (int i = 0; i < rings; i++) {
//			double phi = (M_PI * (i + offset)) / (fragments/2);
				double phi = (M_PI * (i + 0.5)) / rings;
				double r = r1 * sin(phi);
				ring[i].z = r1 * cos(phi);
				ring[i].points = new point2d[fragments];
				generate_circle(ring[i].points, r, fragments);
			}

			p->append_poly();
			for (int i = 0; i < fragments; i++)
				p->append_vertex(ring[0].points[i].x, ring[0].points[i].y, ring[0].z);

			for (int i = 0; i < rings-1; i++) {
				ring_s *r1 = &ring[i];
				ring_s *r2 = &ring[i+1];
				int r1i = 0, r2i = 0;
				while (r1i < fragments || r2i < fragments)
				{
					if (r1i >= fragments)
						goto sphere_next_r2;
					if (r2i >= fragments)
						goto sphere_next_r1;
					if ((double)r1i / fragments <
							(double)r2i / fragments)
					{
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
			for (int i = 0; i < fragments; i++)
				p->insert_vertex(ring[rings-1].points[i].x, 
												 ring[rings-1].points[i].y, 
												 ring[rings-1].z);

			for (int i = 0; i < rings; i++) {
				delete[] ring[i].points;
			}
			delete[] ring;
		}
	}
		break;
	case CYLINDER: {
		PolySet *p = new PolySet(3,true);
		g = p;
		if (this->h > 0 && !std::isinf(this->h) &&
				this->r1 >=0 && this->r2 >= 0 && (this->r1 > 0 || this->r2 > 0) &&
				!std::isinf(this->r1) && !std::isinf(this->r2)) {
			int fragments = Calc::get_fragments_from_r(std::fmax(this->r1, this->r2), this->fn, this->fs, this->fa);

			double z1, z2;
			if (this->center) {
				z1 = -this->h/2;
				z2 = +this->h/2;
			} else {
				z1 = 0;
				z2 = this->h;
			}

			point2d *circle1 = new point2d[fragments];
			point2d *circle2 = new point2d[fragments];

			generate_circle(circle1, r1, fragments);
			generate_circle(circle2, r2, fragments);
		
			for (int i=0; i<fragments; i++) {
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
				for (int i=0; i<fragments; i++)
					p->insert_vertex(circle1[i].x, circle1[i].y, z1);
			}

			if (this->r2 > 0) {
				p->append_poly();
				for (int i=0; i<fragments; i++)
					p->append_vertex(circle2[i].x, circle2[i].y, z2);
			}

			delete[] circle1;
			delete[] circle2;
		}
	}
		break;
	case POLYHEDRON: {
		PolySet *p = new PolySet(3);
		g = p;
		p->setConvexity(this->convexity);
		for (size_t i=0; i<this->faces->toVector().size(); i++)
		{
			p->append_poly();
			const Value::VectorType &vec = this->faces->toVector()[i]->toVector();
			for (size_t j=0; j<vec.size(); j++) {
				size_t pt = vec[j]->toDouble();
				if (pt < this->points->toVector().size()) {
					double px, py, pz;
					if (!this->points->toVector()[pt]->getVec3(px, py, pz) ||
							std::isinf(px) || std::isinf(py) || std::isinf(pz)) {
						PRINTB("ERROR: Unable to convert point at index %d to a vec3 of numbers", j);
						return p;
					}
					p->insert_vertex(px, py, pz);
				}
			}
		}
	}
		break;
	case SQUARE: {
		Polygon2d *p = new Polygon2d();
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
			o.vertices.resize(4);
			o.vertices[0] = v1;
			o.vertices[1] = Vector2d(v2[0], v1[1]);
			o.vertices[2] = v2;
			o.vertices[3] = Vector2d(v1[0], v2[1]);
			p->addOutline(o);
		}
		p->setSanitized(true);
	}
		break;
	case CIRCLE: {
		Polygon2d *p = new Polygon2d();
		g = p;
		if (this->r1 > 0 && !std::isinf(this->r1))	{
			int fragments = Calc::get_fragments_from_r(this->r1, this->fn, this->fs, this->fa);

			Outline2d o;
			o.vertices.resize(fragments);
			for (int i=0; i < fragments; i++) {
				double phi = (M_PI*2*i) / fragments;
				o.vertices[i] = Vector2d(this->r1*cos(phi), this->r1*sin(phi));
			}
			p->addOutline(o);
		}
		p->setSanitized(true);
	}
		break;
	case POLYGON:	{
			Polygon2d *p = new Polygon2d();
			g = p;

			Outline2d outline;
			double x,y;
			const Value::VectorType &vec = this->points->toVector();
			for (unsigned int i=0;i<vec.size();i++) {
				const Value &val = *vec[i];
				if (!val.getVec2(x, y) || std::isinf(x) || std::isinf(y)) {
					PRINTB("ERROR: Unable to convert point %s at index %d to a vec2 of numbers", 
								 val.toString() % i);
					return p;
				}
				outline.vertices.push_back(Vector2d(x, y));
			}

			if (this->paths->toVector().size() == 0 && outline.vertices.size() > 2) {
				p->addOutline(outline);
			}
			else {
				for(const auto &polygon : this->paths->toVector()) {
					Outline2d curroutline;
					for(const auto &index : polygon->toVector()) {
						unsigned int idx = index->toDouble();
						if (idx < outline.vertices.size()) {
							curroutline.vertices.push_back(outline.vertices[idx]);
						}
						// FIXME: Warning on out of bounds?
					}
					p->addOutline(curroutline);
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
	std::stringstream stream;

	stream << this->name();

	switch (this->type) {
	case CUBE:
		stream << "(size = [" << this->x << ", " << this->y << ", " << this->z << "], "
					 <<	"center = " << (center ? "true" : "false") << ")";
		break;
	case SPHERE:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", r = " << this->r1 << ")";
			break;
	case CYLINDER:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", h = " << this->h << ", r1 = " << this->r1
					 << ", r2 = " << this->r2 << ", center = " << (center ? "true" : "false") << ")";
			break;
	case POLYHEDRON:
		stream << "(points = " << *this->points
					 << ", faces = " << *this->faces
					 << ", convexity = " << this->convexity << ")";
			break;
	case SQUARE:
		stream << "(size = [" << this->x << ", " << this->y << "], "
					 << "center = " << (center ? "true" : "false") << ")";
			break;
	case CIRCLE:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", r = " << this->r1 << ")";
		break;
	case POLYGON:
		stream << "(points = " << *this->points << ", paths = " << *this->paths << ", convexity = " << this->convexity << ")";
			break;
	default:
		assert(false);
	}

	return stream.str();
}
 

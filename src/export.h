#pragma once

#include <iostream>
#include "Tree.h"
#include "Camera.h"
#include "memory.h"

#ifdef ENABLE_CGAL

enum FileFormat {
	OPENSCAD_STL,
	OPENSCAD_OFF,
	OPENSCAD_AMF,
	OPENSCAD_DXF,
	OPENSCAD_SVG
};

// void exportFile(const class Geometry *root_geom, std::ostream &output, FileFormat format);
void exportFileByName(const class Geometry *root_geom, FileFormat format,
	const char *name2open, const char *name2display);
void export_png(shared_ptr<const class Geometry> root_geom, Camera &c, std::ostream &output);

void export_stl(const class CGAL_Nef_polyhedron *root_N, std::ostream &output);
void export_stl(const class PolySet &ps, std::ostream &output);
void export_off(const CGAL_Nef_polyhedron *root_N, std::ostream &output);
void export_off(const class PolySet &ps, std::ostream &output);
void export_amf(const class CGAL_Nef_polyhedron *root_N, std::ostream &output);
void export_amf(const class PolySet &ps, std::ostream &output);
void export_dxf(const class Polygon2d &poly, std::ostream &output);
void export_svg(const class Polygon2d &poly, std::ostream &output);
void export_png(const CGAL_Nef_polyhedron *root_N, Camera &c, std::ostream &output);
void export_png_with_opencsg(Tree &tree, Camera &c, std::ostream &output);
void export_png_with_throwntogether(Tree &tree, Camera &c, std::ostream &output);

#endif // ENABLE_CGAL

#ifdef DEBUG
void export_stl(const class PolySet &ps, std::ostream &output);
#endif

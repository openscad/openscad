#pragma once

#include <iostream>
#include "Tree.h"
#include "Camera.h"

enum FileFormat {
	OPENSCAD_STL,
	OPENSCAD_OFF,
	OPENSCAD_AMF,
	OPENSCAD_DXF,
	OPENSCAD_SVG
};

void exportFileByName(const class Geometry *root_geom, FileFormat format,
	const char *name2open, const char *name2display);

void export_stl(const Geometry *geom, std::ostream &output);
void export_off(const Geometry *geom, std::ostream &output);
void export_amf(const Geometry *geom, std::ostream &output);
void export_dxf(const Geometry *geom, std::ostream &output);
void export_svg(const Geometry *geom, std::ostream &output);

void export_png(const class Geometry *root_geom, Camera &c, std::ostream &output);
void export_png(const class CGAL_Nef_polyhedron *root_N, Camera &c, std::ostream &output);
void export_png_with_opencsg(Tree &tree, Camera &c, std::ostream &output);
void export_png_with_throwntogether(Tree &tree, Camera &c, std::ostream &output);

#pragma once

#include <iostream>
#include "Tree.h"
#include "Camera.h"
#include "memory.h"

enum class FileFormat {
	STL,
	OFF,
	AMF,
	DXF,
	SVG,
	NEFDBG,
	NEF3
};

void exportFileByName(const shared_ptr<const class Geometry> &root_geom, FileFormat format,
											const char *name2open, const char *name2display);

void export_stl(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_off(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_amf(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_dxf(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_svg(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_nefdbg(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_nef3(const shared_ptr<const Geometry> &geom, std::ostream &output);

// void exportFile(const class Geometry *root_geom, std::ostream &output, FileFormat format);

bool export_png(const shared_ptr<const class Geometry> &root_geom, Camera &c, std::ostream &output);
bool export_png(const shared_ptr<const class CGAL_Nef_polyhedron> &root_N, Camera &c, std::ostream &output);
bool export_png_with_opencsg(Tree &tree, Camera &c, std::ostream &output);
bool export_png_with_throwntogether(Tree &tree, Camera &c, std::ostream &output);

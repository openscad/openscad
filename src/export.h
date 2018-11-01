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

enum class Previewer { OPENCSG, THROWNTOGETHER };
enum class RenderType { GEOMETRY, CGAL, OPENCSG, THROWNTOGETHER };

struct ViewOption {
	const std::string name;
	bool& value;
};

struct ViewOptions {
	bool showAxes;
	bool showScaleMarkers;
	bool showEdges;
	bool showWireFrame;
	bool showCrosshairs;
	Previewer previewer{Previewer::OPENCSG};
	RenderType renderer{RenderType::OPENCSG};
	Camera camera;

	const std::vector<ViewOption> optionList{
		ViewOption{"axes", showAxes},
		ViewOption{"scales", showScaleMarkers},
		ViewOption{"edges", showEdges},
		ViewOption{"wireframe", showWireFrame},
		ViewOption{"crosshairs", showCrosshairs}
	};

	const std::vector<std::string> names() {
		std::vector<std::string> names;
		std::transform(optionList.begin(), optionList.end(), names.end(), [](const ViewOption o){ return o.name; });
		return names;
	}
};

bool export_png(const shared_ptr<const class Geometry> &root_geom, ViewOptions options, std::ostream &output);
bool export_preview_png(Tree &tree, ViewOptions options, std::ostream &output);

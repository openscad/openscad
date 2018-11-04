#pragma once

#include <iostream>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include "Tree.h"
#include "Camera.h"
#include "memory.h"


enum class FileFormat {
	STL,
	OFF,
	AMF,
	_3MF,
	DXF,
	SVG,
	NEFDBG,
	NEF3
};

void exportFileByName(const shared_ptr<const class Geometry> &root_geom, FileFormat format,
											const char *name2open, const char *name2display);

void export_stl(const shared_ptr<const Geometry> &geom, std::ostream &output);
void export_3mf(const shared_ptr<const Geometry> &geom, std::ostream &output);
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
	Previewer previewer{Previewer::OPENCSG};
	RenderType renderer{RenderType::OPENCSG};
	Camera camera;

	std::map<std::string, bool> flags{
		{"axes", false},
		{"scales", false},
		{"edges", false},
		{"wireframe", false},
		{"crosshairs", false},
	};

	const std::vector<std::string> names() {
		std::vector<std::string> names;
		boost::copy(flags | boost::adaptors::map_keys, std::back_inserter(names));
		return names;
	}

	bool &operator[](const std::string &name) {
		return flags.at(name);
	}

	bool operator[](const std::string &name) const {
		return flags.at(name);
	}
	
};

bool export_png(const shared_ptr<const class Geometry> &root_geom, ViewOptions options, std::ostream &output);
bool export_preview_png(Tree &tree, ViewOptions options, std::ostream &output);

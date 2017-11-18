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
#include "polyset.h"
#include "evalcontext.h"
#include "builtin.h"
#include "printutils.h"
#include "fileutils.h"
#include "handle_dep.h"
#include "ext/lodepng/lodepng.h"

#include <cstdint>
#include <array>
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

class SurfaceModule : public AbstractModule
{
public:
	SurfaceModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

typedef std::unordered_map<std::pair<int,int>, double, boost::hash<std::pair<int,int>>> img_data_t;

class SurfaceNode : public LeafNode
{
public:
	VISITABLE();
	SurfaceNode(const ModuleInstantiation *mi) : LeafNode(mi) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "surface"; }

	Filename filename;
	bool center;
	bool invert;
	int convexity;
	
	virtual const Geometry *createGeometry() const;
private:
	void convert_image(img_data_t &data, std::vector<uint8_t> &img, unsigned int width, unsigned int height) const;
	bool is_png(std::vector<uint8_t> &img) const;
	img_data_t read_dat(std::string filename) const;
	img_data_t read_png_or_dat(std::string filename) const;
};

AbstractNode *SurfaceModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	auto node = new SurfaceNode(inst);
	node->center = false;
	node->invert = false;
	node->convexity = 1;

	AssignmentList args{Assignment("file"), Assignment("center"), Assignment("convexity")};

	Context c(ctx);
	c.setVariables(args, evalctx);

	auto fileval = c.lookup_variable("file");
	auto filename = lookup_file(fileval->isUndefined() ? "" : fileval->toString(), inst->path(), c.documentPath());
	node->filename = filename;
	handle_dep(fs::path(filename).generic_string());

	auto center = c.lookup_variable("center", true);
	if (center->type() == Value::ValueType::BOOL) {
		node->center = center->toBool();
	}

	auto convexity = c.lookup_variable("convexity", true);
	if (convexity->type() == Value::ValueType::NUMBER) {
		node->convexity = static_cast<int>(convexity->toDouble());
	}

	auto invert = c.lookup_variable("invert", true);
	if (invert->type() == Value::ValueType::BOOL) {
		node->invert = invert->toBool();
	}

	return node;
}

void SurfaceNode::convert_image(img_data_t &data, std::vector<uint8_t> &img, unsigned int width, unsigned int height) const
{
	for (unsigned int y = 0;y < height;y++) {
		for (unsigned int x = 0;x < width;x++) {
			long idx = 4 * (y * width + x);
			double pixel = 0.2126 * img[idx] + 0.7152 * img[idx + 1] + 0.0722 * img[idx + 2];
			double z = 100.0/255 * (invert ? 1 - pixel : pixel);
			data[std::make_pair(height - 1 - y, x)] = z;
		}
	}
}

bool SurfaceNode::is_png(std::vector<uint8_t> &png) const
{
	return (png.size() >= 8 &&
					std::memcmp(png.data(),
											std::array<uint8_t, 8>({{0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}}).data(), 8) == 0);
}

img_data_t SurfaceNode::read_png_or_dat(std::string filename) const
{
	img_data_t data;
	std::vector<uint8_t> png;
	
	lodepng::load_file(png, filename);
	
	if (!is_png(png)) {
		png.clear();
		return read_dat(filename);
	}
	
	unsigned int width, height;
	std::vector<uint8_t> img;
	auto error = lodepng::decode(img, width, height, png);
	if (error) {
		PRINTB("ERROR: Can't read PNG image '%s'", filename);
		data.clear();
		return data;
	}
	
	convert_image(data, img, width, height);
	
	return data;
}

img_data_t SurfaceNode::read_dat(std::string filename) const
{
	img_data_t data;
	std::ifstream stream(filename.c_str());

	if (!stream.good()) {
		PRINTB("WARNING: Can't open DAT file '%s'.", filename);
		return data;
	}

	int lines = 0, columns = 0;
	double min_val = 0;

	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	boost::char_separator<char> sep(" \t");

	while (!stream.eof()) {
		std::string line;
		while (!stream.eof() && (line.size() == 0 || line[0] == '#')) {
			std::getline(stream, line);
			boost::trim(line);
		}
		if (line.size() == 0 && stream.eof()) break;

		int col = 0;
		tokenizer tokens(line, sep);
		try {
			for(const auto &token : tokens) {
				auto v = boost::lexical_cast<double>(token);
				data[std::make_pair(lines, col++)] = v;
				if (col > columns) columns = col;
				min_val = std::min(v-1, min_val);
			}
		}
		catch (const boost::bad_lexical_cast &blc) {
			if (!stream.eof()) {
				PRINTB("WARNING: Illegal value in '%s': %s", filename % blc.what());
			}
			break;
  	}
		lines++;
	}
	
	return data;
}

const Geometry *SurfaceNode::createGeometry() const
{
	auto data = read_png_or_dat(filename);

	auto p = new PolySet(3);
	p->setConvexity(convexity);
	
	int lines = 0;
	int columns = 0;
	double min_val = 0;
	for (const auto &entry : data) {
		lines = std::max(lines, entry.first.first + 1);
		columns = std::max(columns, entry.first.second + 1);
		min_val = std::min(entry.second - 1, min_val);
	}

	double ox = center ? -(columns-1)/2.0 : 0;
	double oy = center ? -(lines-1)/2.0 : 0;

	for (int i = 1; i < lines; i++)
	for (int j = 1; j < columns; j++)
	{
		double v1 = data[std::make_pair(i-1, j-1)];
		double v2 = data[std::make_pair(i-1, j)];
		double v3 = data[std::make_pair(i, j-1)];
		double v4 = data[std::make_pair(i, j)];
		double vx = (v1 + v2 + v3 + v4) / 4;

		p->append_poly();
		p->append_vertex(ox + j-1, oy + i-1, v1);
		p->append_vertex(ox + j, oy + i-1, v2);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);

		p->append_poly();
		p->append_vertex(ox + j, oy + i-1, v2);
		p->append_vertex(ox + j, oy + i, v4);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);

		p->append_poly();
		p->append_vertex(ox + j, oy + i, v4);
		p->append_vertex(ox + j-1, oy + i, v3);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);

		p->append_poly();
		p->append_vertex(ox + j-1, oy + i, v3);
		p->append_vertex(ox + j-1, oy + i-1, v1);
		p->append_vertex(ox + j-0.5, oy + i-0.5, vx);
	}

	for (int i = 1; i < lines; i++)
	{
		p->append_poly();
		p->append_vertex(ox + 0, oy + i-1, min_val);
		p->append_vertex(ox + 0, oy + i-1, data[std::make_pair(i-1, 0)]);
		p->append_vertex(ox + 0, oy + i, data[std::make_pair(i, 0)]);
		p->append_vertex(ox + 0, oy + i, min_val);

		p->append_poly();
		p->insert_vertex(ox + columns-1, oy + i-1, min_val);
		p->insert_vertex(ox + columns-1, oy + i-1, data[std::make_pair(i-1, columns-1)]);
		p->insert_vertex(ox + columns-1, oy + i, data[std::make_pair(i, columns-1)]);
		p->insert_vertex(ox + columns-1, oy + i, min_val);
	}

	for (int i = 1; i < columns; i++)
	{
		p->append_poly();
		p->insert_vertex(ox + i-1, oy + 0, min_val);
		p->insert_vertex(ox + i-1, oy + 0, data[std::make_pair(0, i-1)]);
		p->insert_vertex(ox + i, oy + 0, data[std::make_pair(0, i)]);
		p->insert_vertex(ox + i, oy + 0, min_val);

		p->append_poly();
		p->append_vertex(ox + i-1, oy + lines-1, min_val);
		p->append_vertex(ox + i-1, oy + lines-1, data[std::make_pair(lines-1, i-1)]);
		p->append_vertex(ox + i, oy + lines-1, data[std::make_pair(lines-1, i)]);
		p->append_vertex(ox + i, oy + lines-1, min_val);
	}

	if (columns > 1 && lines > 1) {
		p->append_poly();
		for (int i = 0; i < columns-1; i++)
			p->insert_vertex(ox + i, oy + 0, min_val);
		for (int i = 0; i < lines-1; i++)
			p->insert_vertex(ox + columns-1, oy + i, min_val);
		for (int i = columns-1; i > 0; i--)
			p->insert_vertex(ox + i, oy + lines-1, min_val);
		for (int i = lines-1; i > 0; i--)
			p->insert_vertex(ox + 0, oy + i, min_val);
	}

	return p;
}

std::string SurfaceNode::toString() const
{
	std::stringstream stream;
	fs::path path{static_cast<std::string>(this->filename)}; // gcc-4.6

	stream << this->name() << "(file = " << this->filename
		<< ", center = " << (this->center ? "true" : "false")
		<< ", invert = " << (this->invert ? "true" : "false")
				 << ", " "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0)
				 << ")";

	return stream.str();
}

void register_builtin_surface()
{
	Builtins::init("surface", new SurfaceModule());
}

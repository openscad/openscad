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
#include "node.h"
#include "polyset.h"
#include "evalcontext.h"
#include "builtin.h"
#include "printutils.h"
#include "fileutils.h"
#include "handle_dep.h" // handle_dep()
#include "visitor.h"
#include "lodepng.h"

#include <sstream>
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
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

typedef boost::unordered_map<std::pair<int,int>,double> img_data_t;

class SurfaceNode : public LeafNode
{
public:
	SurfaceNode(const ModuleInstantiation *mi) : LeafNode(mi) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "surface"; }

	Filename filename;
	bool center;
	bool invert;
	int convexity;
	
	virtual Geometry *createGeometry() const;
private:
	void convert_image(img_data_t &data, std::vector<unsigned char> &img, unsigned int width, unsigned int height) const;
	bool is_png(std::vector<unsigned char> &img) const;
	img_data_t read_dat(std::string filename) const;
	img_data_t read_png_or_dat(std::string filename) const;
};

AbstractNode *SurfaceModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	SurfaceNode *node = new SurfaceNode(inst);
	node->center = false;
	node->invert = false;
	node->convexity = 1;

	AssignmentList args;
	args += Assignment("file"), Assignment("center"), Assignment("convexity");

	Context c(ctx);
	c.setVariables(args, evalctx);

	ValuePtr fileval = c.lookup_variable("file");
	node->filename = lookup_file(fileval->isUndefined() ? "" : fileval->toString(), inst->path(), c.documentPath());

	ValuePtr center = c.lookup_variable("center", true);
	if (center->type() == Value::BOOL) {
		node->center = center->toBool();
	}

	ValuePtr convexity = c.lookup_variable("convexity", true);
	if (convexity->type() == Value::NUMBER) {
		node->convexity = (int)convexity->toDouble();
	}

	ValuePtr invert = c.lookup_variable("invert", true);
	if (invert->type() == Value::BOOL) {
		node->invert = invert->toBool();
	}

	return node;
}

void SurfaceNode::convert_image(img_data_t &data, std::vector<unsigned char> &img, unsigned int width, unsigned int height) const
{
	double z_min = 100000;
	double z_max = 0;
	for (unsigned long idx = 0;idx < img.size();idx += 4) {
		// sRGB luminance, see http://en.wikipedia.org/wiki/Grayscale
		double z = 0.2126 * img[idx] + 0.7152 * img[idx + 1] + 0.0722 * img[idx + 2];
		if (z < z_min) {
			z_min = z;
		}
		if (z > z_max) {
			z_max = z;
		}
	}

	double h = 100;
	double scale = h / (z_max - z_min);
	for (unsigned int y = 0;y < height;y++) {
		for (unsigned int x = 0;x < width;x++) {
			long idx = 4 * (y * width + x);
			double pixel = 0.2126 * img[idx] + 0.7152 * img[idx + 1] + 0.0722 * img[idx + 2];
			double z = scale * (pixel - z_min);
			if (invert) {
				z = h - z;
			}
			data[std::make_pair(height - y, x)] = z;
			
		}
	}
}

bool SurfaceNode::is_png(std::vector<unsigned char> &png) const
{
	return (png.size() >= 8)
		&& (png[0] == 0x89)
		&& (png[1] == 0x50)
		&& (png[2] == 0x4e)
		&& (png[3] == 0x47)
		&& (png[4] == 0x0d)
		&& (png[5] == 0x0a)
		&& (png[6] == 0x1a)
		&& (png[7] == 0x0a);
}

img_data_t SurfaceNode::read_png_or_dat(std::string filename) const
{
	img_data_t data;
	std::vector<unsigned char> png;
	
	lodepng::load_file(png, filename);
	
	if (!is_png(png)) {
		png.clear();
		return read_dat(filename);
	}
	
	unsigned int width, height;
	std::vector<unsigned char> img;
	unsigned error = lodepng::decode(img, width, height, png);
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

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
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
			BOOST_FOREACH(const std::string &token, tokens) {
				double v = boost::lexical_cast<double>(token);
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

Geometry *SurfaceNode::createGeometry() const
{
	img_data_t data = read_png_or_dat(filename);

	PolySet *p = new PolySet(3);
	p->setConvexity(convexity);
	
	int lines = 0;
	int columns = 0;
	double min_val = std::numeric_limits<double>::max();
	for (img_data_t::iterator it = data.begin();it != data.end();it++) {
		lines = std::max(lines, (*it).first.first + 1);
		columns = std::max(columns, (*it).first.second + 1);
		min_val = std::min((*it).second - 1, min_val);
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

	p->append_poly();
	for (int i = 0; i < columns-1; i++)
		p->insert_vertex(ox + i, oy + 0, min_val);
	for (int i = 0; i < lines-1; i++)
		p->insert_vertex(ox + columns-1, oy + i, min_val);
	for (int i = columns-1; i > 0; i--)
		p->insert_vertex(ox + i, oy + lines-1, min_val);
	for (int i = lines-1; i > 0; i--)
		p->insert_vertex(ox + 0, oy + i, min_val);

	return p;
}

std::string SurfaceNode::toString() const
{
	std::stringstream stream;
	fs::path path((std::string)this->filename);

	stream << this->name() << "(file = " << this->filename
		<< ", center = " << (this->center ? "true" : "false")
		<< ", invert = " << (this->invert ? "true" : "false")
#ifndef OPENSCAD_TESTING
		// timestamp is needed for caching, but disturbs the test framework
				 << ", " "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0)
#endif
				 << ")";

	return stream.str();
}

void register_builtin_surface()
{
	Builtins::init("surface", new SurfaceModule());
}

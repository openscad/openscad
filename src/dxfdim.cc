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

#include "dxfdim.h"
#include "value.h"
#include "function.h"
#include "dxfdata.h"
#include "builtin.h"
#include "printutils.h"
#include "fileutils.h"
#include "evalcontext.h"
#include "handle_dep.h"

#include <cmath>
#include <sstream>
#include <cstdint>

#include <boost/filesystem.hpp>
std::unordered_map<std::string, ValuePtr> dxf_dim_cache;
std::unordered_map<std::string, ValuePtr> dxf_cross_cache;
namespace fs = boost::filesystem;

ValuePtr builtin_dxf_dim(const Context *ctx, const EvalContext *evalctx)
{
	std::string filename;
	std::string layername;
	std::string name;
	double xorigin = 0;
	double yorigin = 0;
	double scale = 1;

	// FIXME: We don't lookup the file relative to where this function was instantiated
	// since the path is only available for ModuleInstantiations, not function expressions.
	// See issue #217
	for (size_t i = 0; i < evalctx->numArgs(); i++) {
		ValuePtr n = evalctx->getArgName(i);
		ValuePtr v = evalctx->getArgValue(i);
		if (evalctx->getArgName(i) == "file") {
			filename = lookup_file(v->toString(),
														 evalctx->documentPath(), ctx->documentPath());
		}
		if (n == "layer") layername = v->toString();
		if (n == "origin") v->getVec2(xorigin, yorigin);
		if (n == "scale") v->getDouble(scale);
		if (n == "name") name = v->toString();
	}

	std::stringstream keystream;
	fs::path filepath(filename);
	uintmax_t filesize = -1;
	time_t lastwritetime = -1;
	if (fs::exists(filepath) && fs::is_regular_file(filepath)) {
		filesize = fs::file_size(filepath);
		lastwritetime = fs::last_write_time(filepath);
	}
	keystream << filename << "|" << layername << "|" << name << "|" << xorigin
						<< "|" << yorigin << "|" << scale << "|" << lastwritetime
						<< "|" << filesize;
	std::string key = keystream.str();
	if (dxf_dim_cache.find(key) != dxf_dim_cache.end()) return dxf_dim_cache.find(key)->second;
	handle_dep(filepath.string());
	DxfData dxf(36, 0, 0, filename, layername, xorigin, yorigin, scale);

	for (size_t i = 0; i < dxf.dims.size(); i++) {
		if (!name.empty() && dxf.dims[i].name != name) continue;

		DxfData::Dim *d = &dxf.dims[i];
		int type = d->type & 7;

		if (type == 0) {
			// Rotated, horizontal or vertical
			double x = d->coords[4][0] - d->coords[3][0];
			double y = d->coords[4][1] - d->coords[3][1];
			double angle = d->angle;
			double distance_projected_on_line = std::fabs(x * cos(angle * M_PI / 180) + y * sin(angle * M_PI / 180));
			return dxf_dim_cache[key] = ValuePtr(distance_projected_on_line);
		}
		else if (type == 1) {
			// Aligned
			double x = d->coords[4][0] - d->coords[3][0];
			double y = d->coords[4][1] - d->coords[3][1];
			return dxf_dim_cache[key] = ValuePtr(sqrt(x * x + y * y));
		}
		else if (type == 2) {
			// Angular
			double a1 = atan2(d->coords[0][0] - d->coords[5][0], d->coords[0][1] - d->coords[5][1]);
			double a2 = atan2(d->coords[4][0] - d->coords[3][0], d->coords[4][1] - d->coords[3][1]);
			return dxf_dim_cache[key] = ValuePtr(std::fabs(a1 - a2) * 180 / M_PI);
		}
		else if (type == 3 || type == 4) {
			// Diameter or Radius
			double x = d->coords[5][0] - d->coords[0][0];
			double y = d->coords[5][1] - d->coords[0][1];
			return dxf_dim_cache[key] = ValuePtr(sqrt(x * x + y * y));
		}
		else if (type == 5) {
			// Angular 3 Point
		}
		else if (type == 6) {
			// Ordinate
			return dxf_dim_cache[key] = ValuePtr((d->type & 64) ? d->coords[3][0] : d->coords[3][1]);
		}

		PRINTB("WARNING: Dimension '%s' in '%s', layer '%s' has unsupported type!",
					 name % filename % layername);
		return ValuePtr::undefined;
	}

	PRINTB("WARNING: Can't find dimension '%s' in '%s', layer '%s'!",
				 name % filename % layername);

	return ValuePtr::undefined;
}

ValuePtr builtin_dxf_cross(const Context *ctx, const EvalContext *evalctx)
{
	std::string filename;
	std::string layername;
	double xorigin = 0;
	double yorigin = 0;
	double scale = 1;

	// FIXME: We don't lookup the file relative to where this function was instantiated
	// since the path is only available for ModuleInstantiations, not function expressions.
	// See isse #217
	for (size_t i = 0; i < evalctx->numArgs(); i++) {
		ValuePtr n = evalctx->getArgName(i);
		ValuePtr v = evalctx->getArgValue(i);
		if (n == "file") filename = ctx->getAbsolutePath(v->toString());
		if (n == "layer") layername = v->toString();
		if (n == "origin") v->getVec2(xorigin, yorigin);
		if (n == "scale") v->getDouble(scale);
	}

	std::stringstream keystream;
	fs::path filepath(filename);
	uintmax_t filesize = -1;
	time_t lastwritetime = -1;
	if (fs::exists(filepath) && fs::is_regular_file(filepath)) {
		filesize = fs::file_size(filepath);
		lastwritetime = fs::last_write_time(filepath);
	}
	keystream << filename << "|" << layername << "|" << xorigin << "|" << yorigin
						<< "|" << scale << "|" << lastwritetime
						<< "|" << filesize;
	std::string key = keystream.str();

	if (dxf_cross_cache.find(key) != dxf_cross_cache.end()) {
		return dxf_cross_cache.find(key)->second;
	}
	handle_dep(filepath.string());
	DxfData dxf(36, 0, 0, filename, layername, xorigin, yorigin, scale);

	double coords[4][2];

	for (size_t i = 0, j = 0; i < dxf.paths.size(); i++) {
		if (dxf.paths[i].indices.size() != 2) continue;
		coords[j][0] = dxf.points[dxf.paths[i].indices[0]][0];
		coords[j++][1] = dxf.points[dxf.paths[i].indices[0]][1];
		coords[j][0] = dxf.points[dxf.paths[i].indices[1]][0];
		coords[j++][1] = dxf.points[dxf.paths[i].indices[1]][1];

		if (j == 4) {
			double x1 = coords[0][0], y1 = coords[0][1];
			double x2 = coords[1][0], y2 = coords[1][1];
			double x3 = coords[2][0], y3 = coords[2][1];
			double x4 = coords[3][0], y4 = coords[3][1];
			double dem = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);
			if (dem == 0) break;
			double ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / dem;
			// double ub = ((x2 - x1)*(y1 - y3) - (y2 - y1)*(x1 - x3)) / dem;
			double x = x1 + ua * (x2 - x1);
			double y = y1 + ua * (y2 - y1);
			Value::VectorType ret;
			ret.push_back(ValuePtr(x));
			ret.push_back(ValuePtr(y));
			return dxf_cross_cache[key] = ValuePtr(ret);
		}
	}

	PRINTB("WARNING: Can't find cross in '%s', layer '%s'!", filename % layername);

	return ValuePtr::undefined;
}

void initialize_builtin_dxf_dim()
{
	Builtins::init("dxf_dim", new BuiltinFunction(&builtin_dxf_dim));
	Builtins::init("dxf_cross", new BuiltinFunction(&builtin_dxf_cross));
}

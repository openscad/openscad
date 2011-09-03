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
#include "context.h"

#include "mathc99.h"
#include <QDateTime>
#include <QFileInfo>
#include <sstream>

boost::unordered_map<std::string,Value> dxf_dim_cache;
boost::unordered_map<std::string,Value> dxf_cross_cache;

Value builtin_dxf_dim(const Context *ctx, const std::vector<std::string> &argnames, const std::vector<Value> &args)
{
	std::string filename;
	std::string layername;
	std::string name;
	double xorigin = 0;
	double yorigin = 0;
	double scale = 1;

	for (size_t i = 0; i < argnames.size() && i < args.size(); i++) {
		if (argnames[i] == "file")
			filename = ctx->get_absolute_path(args[i].text);
		if (argnames[i] == "layer")
			layername = args[i].text;
		if (argnames[i] == "origin")
			args[i].getv2(xorigin, yorigin);
		if (argnames[i] == "scale")
			args[i].getnum(scale);
		if (argnames[i] == "name")
			name = args[i].text;
	}

	QFileInfo fileInfo(QString::fromStdString(filename));

	std::stringstream keystream;
	keystream << filename << "|" << layername << "|" << name << "|" << xorigin
						<< "|" << yorigin <<"|" << scale << "|" << fileInfo.lastModified().toTime_t() 
						<< "|" << fileInfo.size();
	std::string key = keystream.str();
	if (dxf_dim_cache.find(key) != dxf_dim_cache.end())
		return dxf_dim_cache.find(key)->second;

	DxfData dxf(36, 0, 0, QString::fromStdString(filename), QString::fromStdString(layername), xorigin, yorigin, scale);

	for (int i = 0; i < dxf.dims.count(); i++)
	{
		if (!name.empty() && dxf.dims[i].name != QString::fromStdString(name))
			continue;

		DxfData::Dim *d = &dxf.dims[i];
		int type = d->type & 7;

		if (type == 0) {
			// Rotated, horizontal or vertical
			double x = d->coords[4][0] - d->coords[3][0];
			double y = d->coords[4][1] - d->coords[3][1];
			double angle = d->angle;
			double distance_projected_on_line = fabs(x * cos(angle*M_PI/180) + y * sin(angle*M_PI/180));
			return dxf_dim_cache[key] = Value(distance_projected_on_line);
		}
		else if (type == 1) {
			// Aligned
			double x = d->coords[4][0] - d->coords[3][0];
			double y = d->coords[4][1] - d->coords[3][1];
			return dxf_dim_cache[key] = Value(sqrt(x*x + y*y));
		}
		else if (type == 2) {
			// Angular
			double a1 = atan2(d->coords[0][0] - d->coords[5][0], d->coords[0][1] - d->coords[5][1]);
			double a2 = atan2(d->coords[4][0] - d->coords[3][0], d->coords[4][1] - d->coords[3][1]);
			return dxf_dim_cache[key] = Value(fabs(a1 - a2) * 180 / M_PI);
		}
		else if (type == 3 || type == 4) {
			// Diameter or Radius
			double x = d->coords[5][0] - d->coords[0][0];
			double y = d->coords[5][1] - d->coords[0][1];
			return dxf_dim_cache[key] = Value(sqrt(x*x + y*y));
		}
		else if (type == 5) {
			// Angular 3 Point
		}
		else if (type == 6) {
			// Ordinate
			return dxf_dim_cache[key] = Value((d->type & 64) ? d->coords[3][0] : d->coords[3][1]);
		}

		PRINTF("WARNING: Dimension `%s' in `%s', layer `%s' has unsupported type!", name.c_str(), filename.c_str(), layername.c_str());
		return Value();
	}

	PRINTF("WARNING: Can't find dimension `%s' in `%s', layer `%s'!", name.c_str(), filename.c_str(), layername.c_str());

	return Value();
}

Value builtin_dxf_cross(const Context *ctx, const std::vector<std::string> &argnames, const std::vector<Value> &args)
{
	std::string filename;
	std::string layername;
	double xorigin = 0;
	double yorigin = 0;
	double scale = 1;

	for (size_t i = 0; i < argnames.size() && i < args.size(); i++) {
		if (argnames[i] == "file")
			filename = ctx->get_absolute_path(args[i].text);
		if (argnames[i] == "layer")
			layername = args[i].text;
		if (argnames[i] == "origin")
			args[i].getv2(xorigin, yorigin);
		if (argnames[i] == "scale")
			args[i].getnum(scale);
	}

	QFileInfo fileInfo(QString::fromStdString(filename));

	std::stringstream keystream;
	keystream << filename << "|" << layername << "|" << xorigin << "|" << yorigin
						<< "|" << scale << "|" << fileInfo.lastModified().toTime_t()
						<< "|" << fileInfo.size();
	std::string key = keystream.str();

	if (dxf_cross_cache.find(key) != dxf_cross_cache.end())
		return dxf_cross_cache.find(key)->second;

	DxfData dxf(36, 0, 0, QString::fromStdString(filename), QString::fromStdString(layername), xorigin, yorigin, scale);

	double coords[4][2];

	for (int i = 0, j = 0; i < dxf.paths.count(); i++) {
		if (dxf.paths[i].points.count() != 2)
			continue;
		coords[j][0] = (*dxf.paths[i].points[0])[0];
		coords[j++][1] = (*dxf.paths[i].points[0])[1];
		coords[j][0] = (*dxf.paths[i].points[1])[0];
		coords[j++][1] = (*dxf.paths[i].points[1])[1];

		if (j == 4) {
			double x1 = coords[0][0], y1 = coords[0][1];
			double x2 = coords[1][0], y2 = coords[1][1];
			double x3 = coords[2][0], y3 = coords[2][1];
			double x4 = coords[3][0], y4 = coords[3][1];
			double dem = (y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1);
			if (dem == 0)
				break;
			double ua = ((x4 - x3)*(y1 - y3) - (y4 - y3)*(x1 - x3)) / dem;
			// double ub = ((x2 - x1)*(y1 - y3) - (y2 - y1)*(x1 - x3)) / dem;
			double x = x1 + ua*(x2 - x1);
			double y = y1 + ua*(y2 - y1);
			Value ret;
			ret.type = Value::VECTOR;
			ret.append(new Value(x));
			ret.append(new Value(y));
			return dxf_cross_cache[key] = ret;
		}
	}

	PRINTF("WARNING: Can't find cross in `%s', layer `%s'!", filename.c_str(), layername.c_str());

	return Value();
}

void initialize_builtin_dxf_dim()
{
	builtin_functions["dxf_dim"] = new BuiltinFunction(&builtin_dxf_dim);
	builtin_functions["dxf_cross"] = new BuiltinFunction(&builtin_dxf_cross);
}


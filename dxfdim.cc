/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "openscad.h"

Value builtin_dxf_dim(const QVector<QString> &argnames, const QVector<Value> &args)
{
	QString filename;
	QString layername;
	QString name;
	double xorigin = 0;
	double yorigin = 0;
	double scale = 1;

	for (int i = 0; i < argnames.count() && i < args.count(); i++) {
		if (argnames[i] == "file")
			filename = args[i].text;
		if (argnames[i] == "layer")
			filename = args[i].text;
		if (argnames[i] == "origin")
			args[i].getv2(xorigin, yorigin);
		if (argnames[i] == "scale")
			args[i].getnum(scale);
		if (argnames[i] == "name")
			name = args[i].text;
	}

	DxfData dxf(36, 0, 0, filename, layername, xorigin, yorigin, scale);

	for (int i = 0; i < dxf.dims.count(); i++)
	{
		if (!name.isNull() && dxf.dims[i].name != name)
			continue;

		DxfData::Dim *d = &dxf.dims[i];
		int type = d->type & 7;

		if (type == 0) {
			// Rotated, horizontal or vertical
			double x = d->coords[4][0] - d->coords[3][0];
			double y = d->coords[4][1] - d->coords[3][1];
			double angle = d->angle;
			double distance_projected_on_line = fabs(x * sin(angle*M_PI/180) + y * cos(angle*M_PI/180));
			return Value(distance_projected_on_line);
		}
		if (type == 1) {
			// Aligned
		}
		if (type == 2) {
			// Angular
		}
		if (type == 3) {
			// Diameter
		}
		if (type == 4) {
			// Radius
		}
		if (type == 5) {
			// Angular 3 Point
		}

		PRINTA("WARINING: Dimension `%1' in `%2', layer `%3' has unsupported type!", name, filename, layername);
		return Value();
	}

	PRINTA("WARINING: Can't find dimension `%1' in `%2', layer `%3'!", name, filename, layername);

	return Value();
}

void initialize_builtin_dxf_dim()
{
	builtin_functions["dxf_dim"] = new BuiltinFunction(&builtin_dxf_dim);
}


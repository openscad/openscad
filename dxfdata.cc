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

#include <QFile>

DxfData::DxfData(double /* fn */, double /* fs */, double /* fa */, QString filename, QString layername)
{
	QFile f(filename);

	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		PRINTF("WARNING: Can't open DXF file `%s'.", filename.toAscii().data());
		return;
	}

	// WARNING: The algorithms used here are extreamly sub-optimal and perform
	// as bad as O(n^3). So for reading large DXF paths one might consider optimizing
	// the code in this function..
	QVector<Line> lines;

	QString mode, layer;
	double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

	while (!f.atEnd())
	{
		QString id_str = QString(f.readLine()).remove("\n");
		QString data = QString(f.readLine()).remove("\n");

		bool status;
		int id = id_str.toInt(&status);

		if (!status)
			break;

		switch (id)
		{
		case 0:
			if (mode == "LINE" && (layername.isNull() || layername == layer)) {
				lines.append(Line(p(x1, y1), p(x2, y2)));
			}
			mode = data;
			break;
		case 8:
			layer = data;
			break;
		case 10:
			x1 = data.toDouble();
			break;
		case 11:
			x2 = data.toDouble();
			break;
		case 20:
			y1 = data.toDouble();
			break;
		case 21:
			y2 = data.toDouble();
			break;
		}
	}

	// extract all open paths
	while (lines.count() > 0)
	{
		int current_line, current_point;

		for (int i = 0; i < lines.count(); i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < lines.count(); k++) {
					if (lines[i].p[j] == lines[k].p[0])
						goto next_open_path_j;
					if (lines[i].p[j] == lines[k].p[1])
						goto next_open_path_j;
				}
				current_line = i;
				current_point = j;
				goto create_open_path;
			next_open_path_j:;
			}
		}

		break;

	create_open_path:
		paths.append(Path());
		Path *this_path = &paths.last();

		this_path->points.append(lines[current_line].p[current_point]);
		while (1) {
			this_path->points.append(lines[current_line].p[!current_point]);
			Point *ref_point = lines[current_line].p[!current_point];
			lines.remove(current_line);
			for (int k = 0; k < lines.count(); k++) {
				if (ref_point == lines[k].p[0]) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_open_path;
				}
				if (ref_point == lines[k].p[1]) {
					current_line = k;
					current_point = 1;
					goto found_next_line_in_open_path;
				}
			}
			break;
		found_next_line_in_open_path:;
		}
	}

	// extract all closed paths
	while (lines.count() > 0)
	{
		int current_line = 0, current_point = 0;

		paths.append(Path());
		Path *this_path = &paths.last();
		this_path->is_closed = true;
		
		this_path->points.append(lines[current_line].p[current_point]);
		while (1) {
			this_path->points.append(lines[current_line].p[!current_point]);
			Point *ref_point = lines[current_line].p[!current_point];
			lines.remove(current_line);
			for (int k = 0; k < lines.count(); k++) {
				if (ref_point == lines[k].p[0]) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_closed_path;
				}
				if (ref_point == lines[k].p[1]) {
					current_line = k;
					current_point = 1;
					goto found_next_line_in_closed_path;
				}
			}
			break;
		found_next_line_in_closed_path:;
		}
	}

	if (paths.count() > 0) {
		double min_x1 = paths[0].points[0]->x;
		int min_x_path = 0;
		for (int i = 0; i < paths.count(); i++) {
			if (!paths[i].is_closed)
				break;
			paths[i].is_inner = true;
			double min_x2 = paths[i].points[0]->x;
			int min_x_point = 0;
			for (int j = 0; j < paths[i].points.count(); j++) {
				if (paths[i].points[j]->x < min_x1) {
					min_x1 = paths[i].points[j]->x;
					min_x_path = i;
				}
				if (paths[i].points[j]->x < min_x2) {
					min_x2 = paths[i].points[j]->x;
					min_x_point = j;
				}
			}
			// rotate points if the path is not in non-standard rotation
			int b = min_x_point;
			int a = b == 0 ? paths[i].points.count() - 1 : b - 1;
			int c = b == paths[i].points.count() - 1 ? 0 : b + 1;
			double ax = paths[i].points[a]->x - paths[i].points[b]->x;
			double ay = paths[i].points[a]->y - paths[i].points[b]->y;
			double cx = paths[i].points[c]->x - paths[i].points[c]->x;
			double cy = paths[i].points[c]->y - paths[i].points[c]->y;
			if (atan2(ax, ay) < atan2(cx, cy)) {
				for (int j = 0; j < paths[i].points.count()/2; j++)
					paths[i].points.swap(j, paths[i].points.count()-1-j);
			}
		}
		paths[min_x_path].is_inner = false;
	}

#if 0
	printf("----- DXF Data -----\n");
	for (int i = 0; i < paths.count(); i++) {
		printf("Path %d (%s, %s):\n", i, paths[i].is_closed ? "closed" : "open", paths[i].is_inner ? "inner" : "outer");
		for (int j = 0; j < paths[i].points.count(); j++)
			printf("  %f %f\n", paths[i].points[j]->x, paths[i].points[j]->y);
	}
	printf("--------------------\n");
#endif
}

DxfData::Point *DxfData::p(double x, double y)
{
	for (int i = 0; i < points.count(); i++) {
		if (abs(points[i].x - x) < 0.01 && abs(points[i].y - y) < 0.01)
			return &points[i];
	}
	points.append(Point(x, y));
	return &points[points.count()-1];
}


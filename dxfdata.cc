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
#include "printutils.h"

#include <QFile>
#include <QTextStream>

struct Line {
	typedef DxfData::Point Point;
	Point *p[2];
	bool disabled;
	Line(Point *p1, Point *p2) { p[0] = p1; p[1] = p2; disabled = false; }
	Line() { p[0] = NULL; p[1] = NULL; disabled = false; }
};

DxfData::DxfData()
{
}

DxfData::DxfData(double fn, double fs, double fa, QString filename, QString layername, double xorigin, double yorigin, double scale)
{
	handle_dep(filename);
	QFile f(filename);

	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		PRINTF("WARNING: Can't open DXF file `%s'.", filename.toAscii().data());
		return;
	}
	QTextStream stream(&f);

	QList<Line> lines;
	Grid2d< QVector<int> > grid;
	QHash< QString, QList<Line> > blockdata;

	bool in_entities_section = false;
	bool in_blocks_section = false;
	QString current_block;

#define ADD_LINE(_x1, _y1, _x2, _y2) do {										\
		double _p1x = _x1, _p1y = _y1, _p2x = _x2, _p2y = _y2;  \
		if (!in_entities_section && !in_blocks_section)         \
			break;                                                \
		if (in_entities_section &&                              \
				!(layername.isNull() || layername == layer))        \
			break;                                                \
		grid.align(_p1x, _p1y);                                 \
		grid.align(_p2x, _p2y);                                 \
		grid.data(_p1x, _p1y).append(lines.count());            \
		grid.data(_p2x, _p2y).append(lines.count());            \
		if (in_entities_section)                                \
			lines.append(Line(p(_p1x, _p1y), p(_p2x, _p2y)));     \
		if (in_blocks_section && !current_block.isNull())       \
			blockdata[current_block].append(                      \
				Line(p(_p1x, _p1y), p(_p2x, _p2y)));								\
	} while (0)

	QString mode, layer, name, iddata;
	int dimtype = 0;
	double coords[7][2];
	QVector<double> xverts;
	QVector<double> yverts;
	double radius = 0, start_angle = 0, stop_angle = 0;

	for (int i = 0; i < 7; i++)
		for (int j = 0; j < 2; j++)
			coords[i][j] = 0;

	QHash<QString, int> unsupported_entities_list;

	while (!stream.atEnd())
	{
		QString id_str = stream.readLine();
		QString data = stream.readLine();

		bool status;
		int id = id_str.toInt(&status);

		if (!status) {
			PRINTA("WARNING: Illegal ID `%1' in `%3'.", id_str, filename);
			break;
		}

		if (id >= 10 && id <= 16) {
			if (id == 11 || id == 12 || id == 16)
				coords[id-10][0] = data.toDouble() * scale;
			else
				coords[id-10][0] = (data.toDouble() - xorigin) * scale;
		}

		if (id >= 20 && id <= 26) {
			if (id == 21 || id == 22 || id == 26)
				coords[id-20][1] = data.toDouble() * scale;
			else
				coords[id-20][1] = (data.toDouble() - yorigin) * scale;
		}

		switch (id)
		{
		case 0:
			if (mode == "SECTION") {
				in_entities_section = iddata == "ENTITIES";
				in_blocks_section = iddata == "BLOCKS";
			}
			else if (mode == "LINE") {
				ADD_LINE(xverts[0], yverts[0], xverts[1], yverts[1]);
			}
			else if (mode == "LWPOLYLINE") {
				assert(xverts.size() == yverts.size());
				// polyline flag is stored in 'dimtype'
				int numverts = xverts.size();
				for (int i=1;i<numverts;i++) {
					ADD_LINE(xverts[i-1], yverts[i-1], xverts[i%numverts], yverts[i%numverts]);
				}
				if (dimtype & 0x01) { // closed polyline
					ADD_LINE(xverts[numverts-1], yverts[numverts-1], xverts[0], yverts[0]);
				}
			}
			else if (mode == "CIRCLE") {
				int n = get_fragments_from_r(radius, fn, fs, fa);
				Point center(xverts[0], yverts[0]);
				for (int i = 0; i < n; i++) {
					double a1 = (2*M_PI*i)/n;
					double a2 = (2*M_PI*(i+1))/n;
					ADD_LINE(cos(a1)*radius + center.x, sin(a1)*radius + center.y,
									 cos(a2)*radius + center.x, sin(a2)*radius + center.y);
				}
			}
			else if (mode == "ARC") {
				Point center(xverts[0], yverts[0]);
				int n = get_fragments_from_r(radius, fn, fs, fa);
				while (start_angle > stop_angle)
					stop_angle += 360.0;
				n = (int)ceil(n * (stop_angle-start_angle) / 360);
				for (int i = 0; i < n; i++) {
					double a1 = ((stop_angle-start_angle)*i)/n;
					double a2 = ((stop_angle-start_angle)*(i+1))/n;
					a1 = (start_angle + a1) * M_PI / 180.0;
					a2 = (start_angle + a2) * M_PI / 180.0;
					ADD_LINE(cos(a1)*radius + center.x, sin(a1)*radius + center.y,
									 cos(a2)*radius + center.x, sin(a2)*radius + center.y);
				}
			}
			else if (mode == "ELLIPSE") {
				// Commented code is meant as documentation of vector math
				while (start_angle > stop_angle) stop_angle += 2 * M_PI;
//				Vector2d center(xverts[0], yverts[0]);
				Point center(xverts[0], yverts[0]);
//				Vector2d ce(xverts[1], yverts[1]);
				Point ce(xverts[1], yverts[1]);
//				double r_major = ce.length();
				double r_major = sqrt(ce.x*ce.x + ce.y*ce.y);
//				double rot_angle = ce.angle();
				double rot_angle;
				{
//					double dot = ce.dot(Vector2d(1.0, 0.0));
					double dot = ce.x;
					double cosval = dot / r_major;
					if (cosval > 1.0) cosval = 1.0;
					if (cosval < -1.0) cosval = -1.0;
					rot_angle = acos(cosval);
					if (ce.y < 0.0) rot_angle = 2 * M_PI - rot_angle;
				}

				// the ratio stored in 'radius; due to the parser code not checking entity type
				double r_minor = r_major * radius;
				double sweep_angle = stop_angle-start_angle;
				int n = get_fragments_from_r(r_major, fn, fs, fa);
				n = (int)ceil(n * sweep_angle / (2 * M_PI));
//				Vector2d p1;
				Point p1;
				for (int i=0;i<=n;i++) {
					double a = (start_angle + sweep_angle*i/n);
//					Vector2d p2(cos(a)*r_major, sin(a)*r_minor);
					Point p2(cos(a)*r_major, sin(a)*r_minor);
//					p2.rotate(rot_angle);
					Point p2_rot(cos(rot_angle)*p2.x - sin(rot_angle)*p2.y,
											 sin(rot_angle)*p2.x + cos(rot_angle)*p2.y);
//					p2 += center;
					p2_rot.x += center.x;
					p2_rot.y += center.y;
					if (i > 0) {
// 						ADD_LINE(p1[0], p1[1], p2[0], p2[1]);
						ADD_LINE(p1.x, p1.y, p2_rot.x, p2_rot.y);
					}
//					p1 = p2;
					p1.x = p2_rot.x;
					p1.y = p2_rot.y;
				}
			}
			else if (mode == "INSERT") {
				int n = blockdata[iddata].size();
				for (int i = 0; i < n; i++) {
					double a = -start_angle * M_PI / 180.0;
					double lx1 = blockdata[iddata][i].p[0]->x;
					double ly1 = blockdata[iddata][i].p[0]->y;
					double lx2 = blockdata[iddata][i].p[1]->x;
					double ly2 = blockdata[iddata][i].p[1]->y;
					double px1 = cos(a)*lx1 + sin(a)*ly1 + xverts[0];
					double py1 = sin(a)*lx1 + cos(a)*ly1 + yverts[0];
					double px2 = cos(a)*lx2 + sin(a)*ly2 + xverts[0];
					double py2 = sin(a)*lx2 + cos(a)*ly2 + yverts[0];
					ADD_LINE(px1, py1, px2, py2);
				}
			}
			else if (mode == "DIMENSION" &&
					(layername.isNull() || layername == layer)) {
				dims.append(Dim());
				dims.last().type = dimtype;
				for (int i = 0; i < 7; i++)
					for (int j = 0; j < 2; j++)
						dims.last().coords[i][j] = coords[i][j];
				dims.last().angle = start_angle;
				dims.last().name = name;
			}
			else if (mode == "BLOCK") {
				current_block = iddata;
			}
			else if (mode == "ENDBLK") {
				current_block = QString();
			}
			else if (mode == "ENDSEC") {
			}
			else if (in_blocks_section || (in_entities_section &&
																(layername.isNull() || layername == layer))) {
					unsupported_entities_list[mode]++;
			}
			mode = data;
			layer = QString();
			name = QString();
			iddata = QString();
			dimtype = 0;
			for (int i = 0; i < 7; i++)
				for (int j = 0; j < 2; j++)
					coords[i][j] = 0;
			xverts.clear();
			yverts.clear();
			radius = start_angle = stop_angle = 0;
			break;
		case 1:
			name = data;
			break;
		case 2:
			iddata = data;
			break;
		case 8:
			layer = data;
			break;
		case 10:
			xverts.append((data.toDouble() - xorigin) * scale);
			break;
		case 11:
			xverts.append((data.toDouble() - xorigin) * scale);
			break;
		case 20:
			yverts.append((data.toDouble() - yorigin) * scale);
			break;
		case 21:
			yverts.append((data.toDouble() - yorigin) * scale);
			break;
		case 40:
			// CIRCLE, ARC: radius
			// ELLIPSE: minor to major ratio
			radius = data.toDouble() * scale;
			break;
		case 41: // for ELLIPSE
		case 50: // for ARC
			start_angle = data.toDouble();
			break;
		case 42: // for ELLIPSE
		case 51: // for ARC
			stop_angle = data.toDouble();
			break;
		case 70:
			// LWPOLYLINE: polyline flag
			// DIMENSION: dimension type
			dimtype = data.toInt();
			break;
		}
	}

	QHashIterator<QString, int> i(unsupported_entities_list);
	while (i.hasNext()) {
		i.next();
		if (layername.isNull()) {
			PRINTA("WARNING: Unsupported DXF Entity `%1' (%2x) in `%3'.",
						 i.key(), QString::number(i.value()), filename);
		} else {
			PRINTA("WARNING: Unsupported DXF Entity `%1' (%2x) in layer `%3' of `%4'.",
						 i.key(), QString::number(i.value()), layername, filename);
		}
	}

	QHash<int, int> enabled_lines;
	for (int i = 0; i < lines.count(); i++) {
		enabled_lines[i] = i;
	}

	// extract all open paths
	while (enabled_lines.count() > 0)
	{
		int current_line, current_point;

		foreach (int i, enabled_lines) {
			for (int j = 0; j < 2; j++) {
				QVector<int> *lv = &grid.data(lines[i].p[j]->x, lines[i].p[j]->y);
				for (int ki = 0; ki < lv->count(); ki++) {
					int k = lv->at(ki);
					if (k == i || lines[k].disabled)
						continue;
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
			lines[current_line].disabled = true;
			enabled_lines.remove(current_line);
			QVector<int> *lv = &grid.data(ref_point->x, ref_point->y);
			for (int ki = 0; ki < lv->count(); ki++) {
				int k = lv->at(ki);
				if (lines[k].disabled)
					continue;
				if (grid.eq(ref_point->x, ref_point->y, lines[k].p[0]->x, lines[k].p[0]->y)) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_open_path;
				}
				if (grid.eq(ref_point->x, ref_point->y, lines[k].p[1]->x, lines[k].p[1]->y)) {
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
	while (enabled_lines.count() > 0)
	{
		int current_line = enabled_lines.begin().value(), current_point = 0;

		paths.append(Path());
		Path *this_path = &paths.last();
		this_path->is_closed = true;
		
		this_path->points.append(lines[current_line].p[current_point]);
		while (1) {
			this_path->points.append(lines[current_line].p[!current_point]);
			Point *ref_point = lines[current_line].p[!current_point];
			lines[current_line].disabled = true;
			enabled_lines.remove(current_line);
			QVector<int> *lv = &grid.data(ref_point->x, ref_point->y);
			for (int ki = 0; ki < lv->count(); ki++) {
				int k = lv->at(ki);
				if (lines[k].disabled)
					continue;
				if (grid.eq(ref_point->x, ref_point->y, lines[k].p[0]->x, lines[k].p[0]->y)) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_closed_path;
				}
				if (grid.eq(ref_point->x, ref_point->y, lines[k].p[1]->x, lines[k].p[1]->y)) {
					current_line = k;
					current_point = 1;
					goto found_next_line_in_closed_path;
				}
			}
			break;
		found_next_line_in_closed_path:;
		}
	}

	fixup_path_direction();

#if 0
	printf("----- DXF Data -----\n");
	for (int i = 0; i < paths.count(); i++) {
		printf("Path %d (%s):\n", i, paths[i].is_closed ? "closed" : "open");
		for (int j = 0; j < paths[i].points.count(); j++)
			printf("  %f %f\n", paths[i].points[j]->x, paths[i].points[j]->y);
	}
	printf("--------------------\n");
	fflush(stdout);
#endif
}

void DxfData::fixup_path_direction()
{
	if (paths.count() > 0) {
		for (int i = 0; i < paths.count(); i++) {
			if (!paths[i].is_closed)
				break;
			paths[i].is_inner = true;
			double min_x = paths[i].points[0]->x;
			int min_x_point = 0;
			for (int j = 0; j < paths[i].points.count(); j++) {
				if (paths[i].points[j]->x < min_x) {
					min_x = paths[i].points[j]->x;
					min_x_point = j;
				}
			}
			// rotate points if the path is in non-standard rotation
			int b = min_x_point;
			int a = b == 0 ? paths[i].points.count() - 2 : b - 1;
			int c = b == paths[i].points.count() - 1 ? 1 : b + 1;
			double ax = paths[i].points[a]->x - paths[i].points[b]->x;
			double ay = paths[i].points[a]->y - paths[i].points[b]->y;
			double cx = paths[i].points[c]->x - paths[i].points[b]->x;
			double cy = paths[i].points[c]->y - paths[i].points[b]->y;
#if 0
			printf("Rotate check:\n");
			printf("  a/b/c indices = %d %d %d\n", a, b, c);
			printf("  b->a vector = %f %f (%f)\n", ax, ay, atan2(ax, ay));
			printf("  b->c vector = %f %f (%f)\n", cx, cy, atan2(cx, cy));
#endif
			if (atan2(ax, ay) < atan2(cx, cy)) {
				for (int j = 0; j < paths[i].points.count()/2; j++)
					paths[i].points.swap(j, paths[i].points.count()-1-j);
			}
		}
	}
}

DxfData::Point *DxfData::p(double x, double y)
{
	points.append(Point(x, y));
	return &points.last();
}


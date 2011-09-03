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

#include "dxfdata.h"
#include "grid.h"
#include "printutils.h"
#include "openscad.h" // handle_dep()

#include <QFile>
#include <QTextStream>
#include <QHash>
#include <QVector>
#include "mathc99.h"
#include <assert.h>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <algorithm>

struct Line {
	int idx[2]; // indices into DxfData::points
	bool disabled;
	Line(int i1 = -1, int i2 = -1) { idx[0] = i1; idx[1] = i2; disabled = false; }
};

DxfData::DxfData()
{
}

/*!
	Reads a layer from the given file, or all layers if layername.empty()
 */
DxfData::DxfData(double fn, double fs, double fa, 
								 const std::string &filename, const std::string &layername, 
								 double xorigin, double yorigin, double scale)
{
	handle_dep(QString::fromStdString(filename)); // Register ourselves as a dependency

	QFile f(QString::fromStdString(filename));
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		PRINTF("WARNING: Can't open DXF file `%s'.", filename.c_str());
		return;
	}
	QTextStream stream(&f);

	Grid2d< std::vector<int> > grid(GRID_COARSE);
	std::vector<Line> lines;                       // Global lines
	QHash< QString, std::vector<Line> > blockdata; // Lines in blocks

	bool in_entities_section = false;
	bool in_blocks_section = false;
	std::string current_block;

#define ADD_LINE(_x1, _y1, _x2, _y2) do {										\
		double _p1x = _x1, _p1y = _y1, _p2x = _x2, _p2y = _y2;  \
		if (!in_entities_section && !in_blocks_section)         \
			break;                                                \
		if (in_entities_section &&                              \
				!(layername.empty() || layername == layer))        \
			break;                                                \
		grid.align(_p1x, _p1y);                                 \
		grid.align(_p2x, _p2y);                                 \
		grid.data(_p1x, _p1y).push_back(lines.size());            \
		grid.data(_p2x, _p2y).push_back(lines.size());            \
		if (in_entities_section)                                \
			lines.push_back(                                         \
				Line(addPoint(_p1x, _p1y), addPoint(_p2x, _p2y)));	\
		if (in_blocks_section && !current_block.empty())       \
			blockdata[QString::fromStdString(current_block)].push_back(	\
				Line(addPoint(_p1x, _p1y), addPoint(_p2x, _p2y)));	\
	} while (0)

	std::string mode, layer, name, iddata;
	int dimtype = 0;
	double coords[7][2]; // Used by DIMENSION entities
	std::vector<double> xverts;
	std::vector<double> yverts;
	double radius = 0;
	double arc_start_angle = 0, arc_stop_angle = 0;
	double ellipse_start_angle = 0, ellipse_stop_angle = 0;

	for (int i = 0; i < 7; i++)
		for (int j = 0; j < 2; j++)
			coords[i][j] = 0;

	QHash<QString, int> unsupported_entities_list;


	//
	// Parse DXF file. Will populate this->points, this->dims, lines and blockdata
	//
	while (!stream.atEnd())
	{
		QString id_str = stream.readLine();
		QString data = stream.readLine();

		bool status;
		int id = id_str.toInt(&status);

		if (!status) {
			PRINTF("WARNING: Illegal ID `%s' in `%s'.", id_str.toUtf8().data(), filename.c_str());
			break;
		}

		if (id >= 10 && id <= 16) {
			if (in_blocks_section)
				coords[id-10][0] = data.toDouble();
			else if (id == 11 || id == 12 || id == 16)
				coords[id-10][0] = data.toDouble() * scale;
			else
				coords[id-10][0] = (data.toDouble() - xorigin) * scale;
		}

		if (id >= 20 && id <= 26) {
			if (in_blocks_section)
				coords[id-20][1] = data.toDouble();
			else if (id == 21 || id == 22 || id == 26)
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
				Vector2d center(xverts[0], yverts[0]);
				for (int i = 0; i < n; i++) {
					double a1 = (2*M_PI*i)/n;
					double a2 = (2*M_PI*(i+1))/n;
					ADD_LINE(cos(a1)*radius + center[0], sin(a1)*radius + center[1],
									 cos(a2)*radius + center[0], sin(a2)*radius + center[1]);
				}
			}
			else if (mode == "ARC") {
				Vector2d center(xverts[0], yverts[0]);
				int n = get_fragments_from_r(radius, fn, fs, fa);
				while (arc_start_angle > arc_stop_angle)
					arc_stop_angle += 360.0;
				n = (int)ceil(n * (arc_stop_angle-arc_start_angle) / 360);
				for (int i = 0; i < n; i++) {
					double a1 = ((arc_stop_angle-arc_start_angle)*i)/n;
					double a2 = ((arc_stop_angle-arc_start_angle)*(i+1))/n;
					a1 = (arc_start_angle + a1) * M_PI / 180.0;
					a2 = (arc_start_angle + a2) * M_PI / 180.0;
					ADD_LINE(cos(a1)*radius + center[0], sin(a1)*radius + center[1],
									 cos(a2)*radius + center[0], sin(a2)*radius + center[1]);
				}
			}
			else if (mode == "ELLIPSE") {
				// Commented code is meant as documentation of vector math
				while (ellipse_start_angle > ellipse_stop_angle) ellipse_stop_angle += 2 * M_PI;
//				Vector2d center(xverts[0], yverts[0]);
				Vector2d center(xverts[0], yverts[0]);
//				Vector2d ce(xverts[1], yverts[1]);
				Vector2d ce(xverts[1], yverts[1]);
//				double r_major = ce.length();
				double r_major = sqrt(ce[0]*ce[0] + ce[1]*ce[1]);
//				double rot_angle = ce.angle();
				double rot_angle;
				{
//					double dot = ce.dot(Vector2d(1.0, 0.0));
					double dot = ce[0];
					double cosval = dot / r_major;
					if (cosval > 1.0) cosval = 1.0;
					if (cosval < -1.0) cosval = -1.0;
					rot_angle = acos(cosval);
					if (ce[1] < 0.0) rot_angle = 2 * M_PI - rot_angle;
				}

				// the ratio stored in 'radius; due to the parser code not checking entity type
				double r_minor = r_major * radius;
				double sweep_angle = ellipse_stop_angle-ellipse_start_angle;
				int n = get_fragments_from_r(r_major, fn, fs, fa);
				n = (int)ceil(n * sweep_angle / (2 * M_PI));
//				Vector2d p1;
				Vector2d p1;
				for (int i=0;i<=n;i++) {
					double a = (ellipse_start_angle + sweep_angle*i/n);
//					Vector2d p2(cos(a)*r_major, sin(a)*r_minor);
					Vector2d p2(cos(a)*r_major, sin(a)*r_minor);
//					p2.rotate(rot_angle);
					Vector2d p2_rot(cos(rot_angle)*p2[0] - sin(rot_angle)*p2[1],
											 sin(rot_angle)*p2[0] + cos(rot_angle)*p2[1]);
//					p2 += center;
					p2_rot[0] += center[0];
					p2_rot[1] += center[1];
					if (i > 0) {
// 						ADD_LINE(p1[0], p1[1], p2[0], p2[1]);
						ADD_LINE(p1[0], p1[1], p2_rot[0], p2_rot[1]);
					}
//					p1 = p2;
					p1[0] = p2_rot[0];
					p1[1] = p2_rot[1];
				}
			}
			else if (mode == "INSERT") {
				// scale is stored in ellipse_start|stop_angle, rotation in arc_start_angle;
				// due to the parser code not checking entity type
				int n = blockdata[QString::fromStdString(iddata)].size();
				for (int i = 0; i < n; i++) {
					double a = arc_start_angle * M_PI / 180.0;
					double lx1 = this->points[blockdata[QString::fromStdString(iddata)][i].idx[0]][0] * ellipse_start_angle;
					double ly1 = this->points[blockdata[QString::fromStdString(iddata)][i].idx[0]][1] * ellipse_stop_angle;
					double lx2 = this->points[blockdata[QString::fromStdString(iddata)][i].idx[1]][0] * ellipse_start_angle;
					double ly2 = this->points[blockdata[QString::fromStdString(iddata)][i].idx[1]][1] * ellipse_stop_angle;
					double px1 = (cos(a)*lx1 - sin(a)*ly1) * scale + xverts[0];
					double py1 = (sin(a)*lx1 + cos(a)*ly1) * scale + yverts[0];
					double px2 = (cos(a)*lx2 - sin(a)*ly2) * scale + xverts[0];
					double py2 = (sin(a)*lx2 + cos(a)*ly2) * scale + yverts[0];
					ADD_LINE(px1, py1, px2, py2);
				}
			}
			else if (mode == "DIMENSION" &&
					(layername.empty() || layername == layer)) {
				this->dims.push_back(Dim());
				this->dims.back().type = dimtype;
				for (int i = 0; i < 7; i++)
					for (int j = 0; j < 2; j++)
						this->dims.back().coords[i][j] = coords[i][j];
				this->dims.back().angle = arc_start_angle;
				this->dims.back().length = radius;
				this->dims.back().name = name;
			}
			else if (mode == "BLOCK") {
				current_block = iddata;
			}
			else if (mode == "ENDBLK") {
				current_block.erase();
			}
			else if (mode == "ENDSEC") {
			}
			else if (in_blocks_section || (in_entities_section &&
					(layername.empty() || layername == layer))) {
				unsupported_entities_list[QString::fromStdString(mode)]++;
			}
			mode = data.toStdString();
			layer.erase();
			name.erase();
			iddata.erase();
			dimtype = 0;
			for (int i = 0; i < 7; i++)
				for (int j = 0; j < 2; j++)
					coords[i][j] = 0;
			xverts.clear();
			yverts.clear();
			radius = arc_start_angle = arc_stop_angle = 0;
			ellipse_start_angle = ellipse_stop_angle = 0;
			if (mode == "INSERT") {
				ellipse_start_angle = ellipse_stop_angle = 1.0; // scale
			}
			break;
		case 1:
			name = data.toStdString();
			break;
		case 2:
			iddata = data.toStdString();
			break;
		case 8:
			layer = data.toStdString();
			break;
		case 10:
			if (in_blocks_section)
				xverts.push_back((data.toDouble()));
			else
				xverts.push_back((data.toDouble() - xorigin) * scale);
			break;
		case 11:
			if (in_blocks_section)
				xverts.push_back((data.toDouble()));
			else
				xverts.push_back((data.toDouble() - xorigin) * scale);
			break;
		case 20:
			if (in_blocks_section)
				yverts.push_back((data.toDouble()));
			else
				yverts.push_back((data.toDouble() - yorigin) * scale);
			break;
		case 21:
			if (in_blocks_section)
				yverts.push_back((data.toDouble()));
			else
				yverts.push_back((data.toDouble() - yorigin) * scale);
			break;
		case 40:
			// CIRCLE, ARC: radius
			// ELLIPSE: minor to major ratio
			// DIMENSION (radial, diameter): Leader length
			radius = data.toDouble();
			if (!in_blocks_section) radius *= scale;
			break;
		case 41:
			// ELLIPSE: start_angle
			// INSERT: X scale
			ellipse_start_angle = data.toDouble();
			break;
		case 50:
			// ARC: start_angle
			// INSERT: rot angle
      // DIMENSION: linear and rotated: angle
			arc_start_angle = data.toDouble();
			break;
		case 42:
			// ELLIPSE: stop_angle
			// INSERT: Y scale
			ellipse_stop_angle = data.toDouble();
			break;
		case 51: // ARC
			arc_stop_angle = data.toDouble();
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
		if (layername.empty()) {
			PRINTA("WARNING: Unsupported DXF Entity `%1' (%2x) in `%3'.",
						 i.key(), QString::number(i.value()), QString::fromStdString(filename));
		} else {
			PRINTA("WARNING: Unsupported DXF Entity `%1' (%2x) in layer `%3' of `%4'.",
						 i.key(), QString::number(i.value()), QString::fromStdString(layername), QString::fromStdString(filename));
		}
	}

	// Extract paths from parsed data

	typedef boost::unordered_map<int, int> LineMap;
	LineMap enabled_lines;
	for (size_t i = 0; i < lines.size(); i++) {
		enabled_lines[i] = i;
	}

	// extract all open paths
	while (enabled_lines.size() > 0)
	{
		int current_line, current_point;

		BOOST_FOREACH(const LineMap::value_type &l, enabled_lines) {
			int idx = l.second;
			for (int j = 0; j < 2; j++) {
				std::vector<int> *lv = &grid.data(this->points[lines[idx].idx[j]][0], this->points[lines[idx].idx[j]][1]);
				for (int ki = 0; ki < lv->size(); ki++) {
					int k = lv->at(ki);
					if (k == idx || lines[k].disabled)
						continue;
					goto next_open_path_j;
				}
				current_line = idx;
				current_point = j;
				goto create_open_path;
			next_open_path_j:;
			}
		}

		break;

	create_open_path:
		this->paths.push_back(Path());
		Path *this_path = &this->paths.back();

		this_path->indices.push_back(lines[current_line].idx[current_point]);
		while (1) {
			this_path->indices.push_back(lines[current_line].idx[!current_point]);
			const Vector2d &ref_point = this->points[lines[current_line].idx[!current_point]];
			lines[current_line].disabled = true;
			enabled_lines.erase(current_line);
			std::vector<int> *lv = &grid.data(ref_point[0], ref_point[1]);
			for (int ki = 0; ki < lv->size(); ki++) {
				int k = lv->at(ki);
				if (lines[k].disabled)
					continue;
				if (grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[0]][0], this->points[lines[k].idx[0]][1])) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_open_path;
				}
				if (grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[1]][0], this->points[lines[k].idx[1]][1])) {
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
	while (enabled_lines.size() > 0)
	{
		int current_line = enabled_lines.begin()->second, current_point = 0;

		this->paths.push_back(Path());
		Path *this_path = &this->paths.back();
		this_path->is_closed = true;
		
		this_path->indices.push_back(lines[current_line].idx[current_point]);
		while (1) {
			this_path->indices.push_back(lines[current_line].idx[!current_point]);
			const Vector2d &ref_point = this->points[lines[current_line].idx[!current_point]];
			lines[current_line].disabled = true;
			enabled_lines.erase(current_line);
			std::vector<int> *lv = &grid.data(ref_point[0], ref_point[1]);
			for (int ki = 0; ki < lv->size(); ki++) {
				int k = lv->at(ki);
				if (lines[k].disabled)
					continue;
				if (grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[0]][0], this->points[lines[k].idx[0]][1])) {
					current_line = k;
					current_point = 0;
					goto found_next_line_in_closed_path;
				}
					if (grid.eq(ref_point[0], ref_point[1], this->points[lines[k].idx[1]][0], this->points[lines[k].idx[1]][1])) {
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
	for (int i = 0; i < this->paths.size(); i++) {
		printf("Path %d (%s):\n", i, this->paths[i].is_closed ? "closed" : "open");
		for (int j = 0; j < this->paths[i].points.size(); j++)
			printf("  %f %f\n", (*this->paths[i].points[j])[0], (*this->paths[i].points[j])[1]);
	}
	printf("--------------------\n");
	fflush(stdout);
#endif
}

/*!
	Ensures that all paths have the same vertex ordering.
	FIXME: CW or CCW?
*/
void DxfData::fixup_path_direction()
{
	for (size_t i = 0; i < this->paths.size(); i++) {
		if (!this->paths[i].is_closed)
			break;
		this->paths[i].is_inner = true;
		double min_x = this->points[this->paths[i].indices[0]][0];
		int min_x_point = 0;
		for (size_t j = 1; j < this->paths[i].indices.size(); j++) {
			if (this->points[this->paths[i].indices[j]][0] < min_x) {
				min_x = this->points[this->paths[i].indices[j]][0];
				min_x_point = j;
			}
		}
		// rotate points if the path is in non-standard rotation
		int b = min_x_point;
		int a = b == 0 ? this->paths[i].indices.size() - 2 : b - 1;
		int c = b == this->paths[i].indices.size() - 1 ? 1 : b + 1;
		double ax = this->points[this->paths[i].indices[a]][0] - this->points[this->paths[i].indices[b]][0];
		double ay = this->points[this->paths[i].indices[a]][1] - this->points[this->paths[i].indices[b]][1];
		double cx = this->points[this->paths[i].indices[c]][0] - this->points[this->paths[i].indices[b]][0];
		double cy = this->points[this->paths[i].indices[c]][1] - this->points[this->paths[i].indices[b]][1];
#if 0
		printf("Rotate check:\n");
		printf("  a/b/c indices = %d %d %d\n", a, b, c);
		printf("  b->a vector = %f %f (%f)\n", ax, ay, atan2(ax, ay));
		printf("  b->c vector = %f %f (%f)\n", cx, cy, atan2(cx, cy));
#endif
		// FIXME: atan2() usually takes y,x. This variant probably makes the path clockwise..
		if (atan2(ax, ay) < atan2(cx, cy)) {
			std::reverse(this->paths[i].indices.begin(), this->paths[i].indices.end());
		}
	}
}

/*!
	Adds a vertex and returns the index into DxfData::points
 */
int DxfData::addPoint(double x, double y)
{
	this->points.push_back(Vector2d(x, y));
	return this->points.size()-1;
}


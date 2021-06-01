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

#include "export.h"
#include "polyset.h"
#include "printutils.h"
#include "Geometry.h"

#include <fstream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

bool canPreview(const FileFormat format) {
	return (format == FileFormat::AST ||
					format == FileFormat::CSG ||
					format == FileFormat::ECHO ||
					format == FileFormat::TERM ||
					format == FileFormat::PNG);
}

void exportFile(const shared_ptr<const Geometry> &root_geom, std::ostream &output, const ExportInfo& exportInfo)
{
	switch (exportInfo.format) {
	case FileFormat::ASCIISTL:
		export_stl(root_geom, output, false);
		break;
	case FileFormat::STL:
		export_stl(root_geom, output, true);
		break;
	case FileFormat::OFF:
		export_off(root_geom, output);
		break;
	case FileFormat::AMF:
		export_amf(root_geom, output);
		break;
	case FileFormat::_3MF:
		export_3mf(root_geom, output);
		break;
	case FileFormat::DXF:
		export_dxf(root_geom, output);
		break;
	case FileFormat::SVG:
		export_svg(root_geom, output);
		break;
	case FileFormat::PDF:
		export_pdf(root_geom, output, exportInfo);
		break;
	case FileFormat::NEFDBG:
		export_nefdbg(root_geom, output);
		break;
	case FileFormat::NEF3:
		export_nef3(root_geom, output);
		break;
	default:
		assert(false && "Unknown file format");
	}
}

void exportFileByNameStdout(const shared_ptr<const Geometry> &root_geom, const ExportInfo& exportInfo)
{
#ifdef _WIN32
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	exportFile(root_geom, std::cout, exportInfo);
}

void exportFileByNameStream(const shared_ptr<const Geometry> &root_geom, const ExportInfo& exportInfo)
{
	std::ios::openmode mode = std::ios::out | std::ios::trunc;
	if (exportInfo.format == FileFormat::_3MF || exportInfo.format == FileFormat::STL || exportInfo.format == FileFormat::PDF) {
		mode |= std::ios::binary;
	}
	std::ofstream fstream(exportInfo.name2open, mode);
	if (!fstream.is_open()) {
		LOG(message_group::None, Location::NONE, "", _("Can't open file \"%1$s\" for export"), exportInfo.name2display);
	} else {
		bool onerror = false;
		fstream.exceptions(std::ios::badbit|std::ios::failbit);
		try {
			exportFile(root_geom, fstream, exportInfo);
		} catch (std::ios::failure&) {
			onerror = true;
		}
		try { // make sure file closed - resources released
			fstream.close();
		} catch (std::ios::failure&) {
			onerror = true;
		}
		if (onerror) {
			LOG(message_group::Error, Location::NONE, "", _("\"%1$s\" write error. (Disk full?)"), exportInfo.name2display);
		}
	}
}

void exportFileByName(const shared_ptr<const Geometry> &root_geom, const ExportInfo& exportInfo)
{
	if (exportInfo.useStdOut) {
		exportFileByNameStdout(root_geom, exportInfo);
	} else {
		exportFileByNameStream(root_geom, exportInfo);
	}
}

namespace Export {

ExportMesh::ExportMesh(const PolySet &ps)
{
	std::vector<std::array<int, 3>> triangleIndices;
	for (const auto &p : ps.polygons) {
		auto pos1 = vertexMap.emplace(std::make_pair<std::array<double, 3>, int>({p[0].x(), p[0].y(), p[0].z()}, vertexMap.size()));
		auto pos2 = vertexMap.emplace(std::make_pair<std::array<double, 3>, int>({p[1].x(), p[1].y(), p[1].z()}, vertexMap.size()));
		auto pos3 = vertexMap.emplace(std::make_pair<std::array<double, 3>, int>({p[2].x(), p[2].y(), p[2].z()}, vertexMap.size()));
		triangleIndices.push_back({pos1.first->second, pos2.first->second, pos3.first->second});
	}

	int index = 0;
	std::map<int, int> indexTranslationMap;
	for (const auto& e : vertexMap) {
		indexTranslationMap.emplace(e.second, index++);
	}

	for (const auto &i : triangleIndices) {
		triangles.emplace_back(indexTranslationMap[i[0]], indexTranslationMap[i[1]], indexTranslationMap[i[2]]);
	}
	std::sort(triangles.begin(), triangles.end(), [](const Triangle& t1, const Triangle& t2) -> bool {
		return t1.key < t2.key;
	});
}

bool ExportMesh::foreach_vertex(const std::function<bool(const std::array<double, 3>&)> callback) const
{
	for (const auto& e : vertexMap) {
		if (!callback(e.first)) {
			return false;
		}
	}
	return true;
}

bool ExportMesh::foreach_triangle(const std::function<bool(const std::array<int, 3>&)> callback) const
{
	for (const auto& t : triangles) {
		if (!callback(t.key)) {
			return false;
		}
	}
	return true;
}

}

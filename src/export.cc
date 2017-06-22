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
#include "printutils.h"
#include "Geometry.h"

#include <fstream>

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

void exportFile(const shared_ptr<const Geometry> &root_geom, std::ostream &output, FileFormat format)
{
	switch (format) {
	case FileFormat::STL:
		export_stl(root_geom, output);
		break;
	case FileFormat::OFF:
		export_off(root_geom, output);
		break;
	case FileFormat::AMF:
		export_amf(root_geom, output);
		break;
	case FileFormat::DXF:
		export_dxf(root_geom, output);
		break;
	case FileFormat::SVG:
		export_svg(root_geom, output);
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

void exportFileByName(const shared_ptr<const Geometry> &root_geom, FileFormat format,
	const char *name2open, const char *name2display)
{
	std::ofstream fstream(name2open);
	if (!fstream.is_open()) {
		PRINTB(_("Can't open file \"%s\" for export"), name2display);
	} else {
		bool onerror = false;
		fstream.exceptions(std::ios::badbit|std::ios::failbit);
		try {
			exportFile(root_geom, fstream, format);
		} catch (std::ios::failure x) {
			onerror = true;
		}
		try { // make sure file closed - resources released
			fstream.close();
		} catch (std::ios::failure x) {
			onerror = true;
		}
		if (onerror) {
			PRINTB(_("ERROR: \"%s\" write error. (Disk full?)"), name2display);
		}
	}
}

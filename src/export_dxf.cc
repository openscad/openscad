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
#include "polyset-utils.h"
#include "dxfdata.h"
#include "settings.h"
#include "version.h"

// 0 = Unitless; 1 = Inches; 2 = Feet; 3 = Miles; 4 = Millimeters;
// 5 = Centimeters; 6 = Meters; 7 = Kilometers; 8 = Microinches;
// 9 = Mils; 10 = Yards; 11 = Angstroms; 12 = Nanometers;
// 13 = Microns; 14 = Decimeters; 15 = Decameters; 16 = Hectometers;
// 17 = Gigameters; 18 = Astronomical units; 19 = Light years; 20 = Parsecs
int getUom()
{
	auto s = Settings::Settings::inst();
	auto uom = s->get(Settings::Settings::dxfUom).toString();
	if (uom == "Unitless")
		return 0;
	else if (uom == "Inches")
		return 1;
	else if (uom == "Feet")
		return 2;
	else if (uom == "Miles")
		return 3;
	else if (uom == "Millimeters")
		return 4;
	else if (uom == "Centimeters")
		return 5;
	else if (uom == "Meters")
		return 6;
	else if (uom == "Kilometers")
		return 7;
	else if (uom == "Microinches")
		return 8;
	else if (uom == "Mils")
		return 9;
	else if (uom == "Yards")
		return 10;
	else
		return 0;
}

//Stream DXF records
//http://help.autodesk.com/view/ACD/2016/ENU/?guid=GUID-D939EA11-0CEC-4636-91A8-756640A031D3
class DxfWriter
{
private:
	std::ostream &os;

public:
	DxfWriter(std::ostream &os) : os(os) {}
	template<typename T>
	void record(int groupCode, T value) {
		os << groupCode << std::endl << value << std::endl;
	}
	void startSection(const std::string name) {
		record(0, "SECTION");
		record(2, name);
	}
	void endSection() { 
		record(0, "ENDSEC"); 
	}
};

/*!
	Saves the current Polygon2d as DXF to the given absolute filename.
	https://knowledge.autodesk.com/search-result/caas/CloudHelp/cloudhelp/2016/ENU/AutoCAD-DXF/files/GUID-5D1DFE5C-94FC-43B7-B535-43001D1662C1-htm.html
 */
void export_dxf(const Polygon2d &poly, std::ostream &output)
{
	DxfWriter w(output);

	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

	// UOM and other AutoCAD DXF format information
	// https://knowledge.autodesk.com/search-result/caas/CloudHelp/cloudhelp/2018/ENU/AutoCAD-DXF/files/GUID-A85E8E67-27CD-4C59-BE61-4DC9FADBE74A-htm.html

	//comment with the version that generated the file
	w.record(999, "OpenSCAD " + openscad_displayversionnumber);

	//Header section
	w.startSection("HEADER");
	// The AutoCAD drawing database version number: AutoCAD 2007
	w.record(9, "$ACADVER");
	w.record(1, "AC1021");
	// Default drawing units for AutoCAD DesignCenter blocks:
	// 70	$INSUNITS
	w.record(9, "$INSUNITS");
	w.record(70, getUom());
	w.endSection();

	// Some importers (e.g. Inkscape) needs a BLOCKS section to be present
	//empty block section
	w.startSection("BLOCKS");
	w.endSection();

	//graphical objects
	w.startSection("ENTITIES");

	for (const auto &o : poly.outlines()) {
		for (unsigned int i = 0; i < o.vertices.size(); i++) {
			//Line entity http://help.autodesk.com/view/OARX/2018/ENU/?guid=GUID-FCEF5726-53AE-4C43-B4EA-C84EB8686A66
			const Vector2d &p1 = o.vertices[i];
			const Vector2d &p2 = o.vertices[(i + 1) % o.vertices.size()];
			double x1 = p1[0];
			double y1 = p1[1];
			double x2 = p2[0];
			double y2 = p2[1];
			w.record(0, "LINE");
			// Some importers (e.g. Inkscape) needs a layer to be specified
			// The [X1 Y1 X2 Y2] order is the most common and can be parsed linearly.
			// Some libraries, like the python libraries dxfgrabber and ezdxf, cannot open [X1 X2 Y1 Y2]
			// order.
			w.record(8, 0);
			w.record(10, x1);
			w.record(20, y1);
			w.record(11, x2);
			w.record(21, y2);
		}
	}
	w.endSection();

	// Some importers (e.g. Inkscape) needs an OBJECTS section with a DICTIONARY entry
	w.startSection("OBJECTS");
	w.record(0, "DICTIONARY");
	w.endSection();

	w.record(0, "EOF");

	setlocale(LC_NUMERIC, ""); // Set default locale
}

void export_dxf(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (dynamic_cast<const PolySet *>(geom.get())) {
		assert(false && "Unsupported file format");
	}
	else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
		export_dxf(*poly, output);
	}
	else {
		assert(false && "Export as DXF for this geometry type is not supported");
	}
}

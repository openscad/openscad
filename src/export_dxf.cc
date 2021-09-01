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

/*!
	Saves the current Polygon2d as DXF to the given absolute filename.
 */
void export_dxf(const Polygon2d &poly, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output

	// Some importers needs a BLOCKS section to be present
	// e.g. Inkscape 1.1 still needs it 
	output  << "  0\n" << "SECTION\n"
			<< "  2\n" << "BLOCKS\n"
			<< "  0\n" << "ENDSEC\n";

	output  << "  0\n" << "SECTION\n"
			<< "  2\n" << "ENTITIES\n";

	// ENTITIES:
    // https://help.autodesk.com/view/ACD/2017/ENU/?guid=GUID-3610039E-27D1-4E23-B6D3-7E60B22BB5BD
	// https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-795d.htm
    // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-7a3d.htm
    
	for(const auto &o : poly.outlines()) {
		switch( o.vertices.size() ) {
		case 1: {
			// POINT: just in case it's supported in the future
			// https://help.autodesk.com/view/ACD/2017/ENU/?guid=GUID-9C6AD32D-769D-4213-85A4-CA9CCB5C5317
            // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-79f2.htm
			const Vector2d &p = o.vertices[0];
			output  << "  0\n" << "POINT\n"
					<< "100\n" << "AcDbEntity\n"
					<< "  8\n" << "0\n" 		// layer 0
					<< "100\n" << "AcDbPoint\n"
					<< " 10\n" << p[0] << "\n"  // x
					<< " 20\n" << p[1] << "\n"; // y					
			} break;
		case 2: {
			// LINE: just in case it's supported in the future
            // https://help.autodesk.com/view/ACD/2017/ENU/?guid=GUID-FCEF5726-53AE-4C43-B4EA-C84EB8686A66
            // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-79fe.htm
			// The [X1 Y1 X2 Y2] order is the most common and can be parsed linearly.
			// Some libraries, like the python libraries dxfgrabber and ezdxf, cannot open [X1 X2 Y1 Y2] order.
			const Vector2d &p1 = o.vertices[0];
			const Vector2d &p2 = o.vertices[1];
			output  << "  0\n" << "LINE\n"
					<< "100\n" << "AcDbEntity\n"
					<< "  8\n" << "0\n" 		 // layer 0
					<< "100\n" << "AcDbLine\n"
					<< " 10\n" << p1[0] << "\n"  // x1
					<< " 20\n" << p1[1] << "\n"  // y1
					<< " 11\n" << p2[0] << "\n"  // x2
					<< " 21\n" << p2[1] << "\n"; // y2				
			} break;
		default:
			// LWPOLYLINE
            // https://help.autodesk.com/view/ACD/2017/ENU/?guid=GUID-748FC305-F3F2-4F74-825A-61F04D757A50
            // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-79fc.htm
			output  << "  0\n" << "LWPOLYLINE\n"
					<< "100\n" << "AcDbEntity\n"
					<< "  8\n" << "0\n" 		            // layer 0
					<< "100\n" << "AcDbPolyline\n"
					<< " 90\n" << o.vertices.size() << "\n" // number of vertices
					<< " 70\n" << "1\n";                    // closed = 1					
			for (unsigned int i=0; i<o.vertices.size(); ++i) {
				const Vector2d &p = o.vertices[i];
				output  << " 10\n" << p[0] << "\n"
						<< " 20\n" << p[1] << "\n";
			}
			break;
		}

		// each segment as separate line
		// for (unsigned int i=0; i<o.vertices.size(); ++i) {
		// 	const Vector2d &p1 = o.vertices[i];
		// 	const Vector2d &p2 = o.vertices[(i+1)%o.vertices.size()];
		// 	output  << "  0\n" << "LINE\n"
		//			<< "100\n" << "AcDbEntity\n"
		//			<< "  8\n" << "0\n" 		 // layer 0
		//			<< "100\n" << "AcDbLine\n"
		// 			<< " 10\n" << p1[0] << "\n"  // x1
		// 			<< " 20\n" << p1[1] << "\n"  // y1
		// 			<< " 11\n" << p2[0] << "\n"  // x2
		// 			<< " 21\n" << p2[1] << "\n"; // y2
		// }
	}

	output << "  0\n" << "ENDSEC\n";

	// Some importers (e.g. Inkscape) needs an OBJECTS section with a DICTIONARY entry.
	// as of Inkscape 1.0, not needed anymore
	// output
	// 	<< "  0\n" << "SECTION\n"
	// 	<< "  2\n" << "OBJECTS\n"
	// 	<< "  0\n" << "DICTIONARY\n"
	// 	<< "  0\n" << "ENDSEC\n";

	output << "  0\n" << "EOF\n";

	setlocale(LC_NUMERIC, ""); // set default locale
}

void export_dxf(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
		for(const auto &item : geomlist->getChildren()) {
			export_dxf(item.second, output);
		}
	} else if (dynamic_pointer_cast<const PolySet>(geom)) {
		assert(false && "Unsupported file format");
	} else if (const auto poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
		export_dxf(*poly, output);
	} else {
		assert(false && "Export as DXF for this geometry type is not supported");
	}
}

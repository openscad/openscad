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

#pragma once

#include <memory>
#include <ostream>

class Geometry;

// =====================================================================
// DXF VERSION SELECTION
// =====================================================================
//
// This enum allows runtime selection of DXF format version.
//
// Usage:
//   export_dxf(geom, output);                       // Default: Legacy
//   export_dxf(geom, output, DxfVersion::Legacy);   // Explicit Legacy
//   export_dxf(geom, output, DxfVersion::R10);      // R10 spec-correct
//   export_dxf(geom, output, DxfVersion::R12);      // R12 with handles
//   export_dxf(geom, output, DxfVersion::R14);      // R14 full object model
//
// Version characteristics:
//
//   Legacy - PRESERVED ORIGINAL BEHAVIOUR (no --dxf-version flag)
//     - Uses LWPOLYLINE for all outlines regardless of AC version string
//     - Identical to original OpenSCAD DXF output
//     - Not strictly spec-valid for pre-R14 readers, but works with
//       most tools in practice
//     - Selected automatically when --dxf-version is not specified
//
//   R10 (AC1006) - SPEC-CORRECT, MAXIMUM COMPATIBILITY
//     - Uses POLYLINE/VERTEX/SEQEND (correct pre-R14 polyline entity)
//     - No handles, no object model
//     - Passes strict validators (ezdxf, etc.)
//     - Best for: Laser cutters, CNC, basic CAM
//
//   R12 (AC1009) - SPEC-CORRECT, MIDDLE GROUND
//     - Uses POLYLINE/VERTEX/SEQEND with handles
//     - Extended header variables ($HANDLING, $HANDSEED, $DWGCODEPAGE)
//     - Passes strict validators
//     - Best for: Mid-range CAM, AutoCAD R12-R13 era tools
//
//   R14 (AC1014) - SPEC-CORRECT, MODERN FORMAT
//     - Uses LWPOLYLINE (correct R14+ lightweight polyline entity)
//     - Full object model (CLASSES, OBJECTS, BLOCK_RECORD)
//     - Mandatory handles throughout
//     - Passes strict validators
//     - Best for: SolidWorks, Inventor, Fusion 360
//
// =====================================================================

enum class DxfVersion {
  Legacy,  // Original OpenSCAD behaviour: LWPOLYLINE for all (DEFAULT)
  R10,     // AC1006 - POLYLINE/VERTEX/SEQEND, no handles, spec-correct
  R12,     // AC1009 - POLYLINE/VERTEX/SEQEND with handles, spec-correct
  R14      // AC1014 - LWPOLYLINE, full object model, spec-correct
};

// Export DXF with explicit version selection
void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output,
                DxfVersion version);

// Export DXF with default version (Legacy - preserves original behaviour)
void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output);

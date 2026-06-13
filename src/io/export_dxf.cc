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

// =====================================================================
// DXF EXPORTER: LEGACY / R10 / R12 / R14 RUNTIME SELECTION
// =====================================================================
//
// This file provides DXF export at four modes, selected at runtime via
// a DxfVersion parameter passed from openscad.cc:
//
//   DxfVersion::Legacy  - Original OpenSCAD behaviour (default, no flag)
//   DxfVersion::R10     - AC1006, spec-correct, POLYLINE entities
//   DxfVersion::R12     - AC1009, spec-correct, POLYLINE + handles
//   DxfVersion::R14     - AC1014, spec-correct, LWPOLYLINE + object model
//
// ENTITY CHOICE BY MODE:
//
//   Legacy:  LWPOLYLINE always (original behaviour, not strictly valid
//            pre-R14 but accepted by most tools)
//
//   R10/R12: POLYLINE / VERTEX / SEQEND  (spec-correct pre-R14 entity)
//            Each outline = one POLYLINE header + N VERTEX records +
//            one SEQEND. R12 adds handles on each of these records.
//
//   R14:     LWPOLYLINE (correct R14+ entity, compact single record)
//
// HANDLE ALLOCATION:
//
//   Legacy/R10: No handles
//   R12:        Handle per POLYLINE + handle per VERTEX + handle per SEQEND
//               handseed = H_ENT_START_R12 + sum(2 + vertex_count) per outline
//   R14:        Handle per LWPOLYLINE entity
//               handseed = H_ENT_START_R14 + outline_count
//
// =====================================================================

#include "io/export.h"
#include "io/export_dxf.h"

#include <cassert>
#include <clocale>
#include <iomanip>
#include <limits>
#include <memory>
#include <ostream>

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"

/*!
    Saves the current Polygon2d as DXF to the given absolute filename.
 */

// =====================================================================
// HANDLE MANAGEMENT (R12 and R14)
// =====================================================================
//
// R12: Handles are OPTIONAL but enabled here via $HANDLING=1.
//      POLYLINE, each VERTEX, and SEQEND each get a handle.
//
// R14: Handles are MANDATORY. Every table entry, block, entity, and
//      object must have a unique handle.
//
// Handle allocation:
//   - Sequential hex values starting at 1
//   - Fixed assignments for tables, blocks, and dictionaries
//   - Dynamic assignment for entities after fixed region
// =====================================================================

// R14 handle assignments: fixed layout for entire object model.
//
// Table order follows the canonical AutoCAD R14 sequence:
//   VPORT, LTYPE, LAYER, STYLE, VIEW, UCS, APPID, DIMSTYLE, BLOCK_RECORD
//
static constexpr int H_VPORT_TABLE_R14 = 0x1;   // VPORT table (must be first)
static constexpr int H_VPORT_ACTIVE_R14 = 0x2;  // *ACTIVE viewport entry
static constexpr int H_LTYPE_TABLE_R14 = 0x3;
static constexpr int H_LTYPE_BYBLOCK_R14 = 0x4;  // ByBlock linetype entry
static constexpr int H_LTYPE_BYLAYER_R14 = 0x5;  // ByLayer linetype entry
static constexpr int H_LTYPE_CONT_R14 = 0x6;     // Continuous linetype entry
static constexpr int H_LAYER_TABLE_R14 = 0x7;
static constexpr int H_LAYER_0_R14 = 0x8;
static constexpr int H_STYLE_TABLE_R14 = 0x9;
static constexpr int H_STYLE_STD_R14 = 0xA;   // "Standard" text style entry
static constexpr int H_VIEW_TABLE_R14 = 0xB;  // VIEW table (empty)
static constexpr int H_UCS_TABLE_R14 = 0xC;   // UCS table (empty)
static constexpr int H_APPID_TABLE_R14 = 0xD;
static constexpr int H_APPID_ACAD_R14 = 0xE;      // ACAD application ID entry
static constexpr int H_DIMSTYLE_TABLE_R14 = 0xF;  // DIMSTYLE table
static constexpr int H_DIMSTYLE_STD_R14 = 0x10;   // "Standard" dim style entry
static constexpr int H_BLOCKREC_TABLE_R14 = 0x11;
static constexpr int H_BLOCKREC_MODEL_R14 = 0x12;
static constexpr int H_BLOCKREC_PAPER_R14 = 0x13;
static constexpr int H_BLOCK_MODEL_R14 = 0x14;
static constexpr int H_ENDBLK_MODEL_R14 = 0x15;
static constexpr int H_BLOCK_PAPER_R14 = 0x16;
static constexpr int H_ENDBLK_PAPER_R14 = 0x17;
static constexpr int H_DICT_ROOT_R14 = 0x18;
static constexpr int H_DICT_GROUP_R14 = 0x19;
static constexpr int H_ENT_START_R14 = 0x1A;  // entity handles start here

// R12 handle assignments: simpler layout, no object model
static constexpr int H_LTYPE_TABLE_R12 = 0x1;
static constexpr int H_LTYPE_CONT_R12 = 0x2;
static constexpr int H_LAYER_TABLE_R12 = 0x3;
static constexpr int H_LAYER_0_R12 = 0x4;
static constexpr int H_STYLE_TABLE_R12 = 0x5;
static constexpr int H_ENT_START_R12 = 0x6;  // entity handles start here

// Emit group 5 (handle) as uppercase hex
static void emit_handle(std::ostream& o, int handle)
{
  o << "  5\n" << std::uppercase << std::hex << handle << std::dec << "\n";
}

// Emit group 330 (owner handle) as uppercase hex (R14 only)
static void emit_owner(std::ostream& o, int handle)
{
  o << "330\n" << std::uppercase << std::hex << handle << std::dec << "\n";
}

// Emit a bare hex value (no group code) for soft-pointer and $HANDSEED
static void emit_hexval(std::ostream& o, int handle)
{
  o << std::uppercase << std::hex << handle << std::dec << "\n";
}

// =====================================================================
// HEADER SECTION GENERATION
// =====================================================================

// Legacy header: byte-for-byte reproduction of original OpenSCAD output.
// Note: $LINMIN/$LINMAX are the original typos (should be $LIMMIN/$LIMMAX)
// and are preserved here intentionally so that Legacy mode is identical to
// the output that existing users rely on.
static void export_dxf_header_Legacy(std::ostream& output, double xMin, double yMin, double xMax,
                                     double yMax)
{
  output << "999\n"
         << "DXF from OpenSCAD\n"  // original comment (no version tag)

         << "  0\nSECTION\n"
         << "  2\nHEADER\n"

         << "  9\n$ACADVER\n"
         << "  1\nAC1006\n"

         << "  9\n$INSBASE\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"

         << "  9\n$EXTMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$EXTMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  9\n$LINMIN\n"  // original typo Ã¢â‚¬â€ preserved
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$LINMAX\n"  // original typo Ã¢â‚¬â€ preserved
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  0\nENDSEC\n";

  // TABLES section
  output << "  0\nSECTION\n"
         << "  2\nTABLES\n";

  // LTYPE table
  output << "  0\nTABLE\n"
         << "  2\nLTYPE\n"
         << " 70\n1\n"
         << "  0\nLTYPE\n"
         << "  2\nCONTINUOUS\n"
         << " 70\n64\n"
         << "  3\nSolid line\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.000000\n"
         << "  0\nENDTAB\n";

  // LAYER table
  output << "  0\nTABLE\n"
         << "  2\nLAYER\n"
         << " 70\n6\n"
         << "  0\nLAYER\n"
         << "  2\n0\n"
         << " 70\n64\n"
         << " 62\n7\n"
         << "  6\nCONTINUOUS\n"
         << "  0\nENDTAB\n";

  // STYLE table
  output << "  0\nTABLE\n"
         << "  2\nSTYLE\n"
         << " 70\n0\n"
         << "  0\nENDTAB\n";

  output << "  0\nENDSEC\n";

  // BLOCKS section (empty)
  output << "  0\nSECTION\n"
         << "  2\nBLOCKS\n"
         << "  0\nENDSEC\n";
}

static void export_dxf_header_R10(std::ostream& output, double xMin, double yMin, double xMax,
                                  double yMax)
{
  output << "999\n"
         << "DXF from OpenSCAD (R10)\n"

         << "  0\nSECTION\n"
         << "  2\nHEADER\n"

         << "  9\n$ACADVER\n"
         << "  1\nAC1006\n"

         << "  9\n$INSBASE\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"

         << "  9\n$EXTMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$EXTMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  9\n$LIMMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$LIMMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  0\nENDSEC\n";

  // TABLES section
  output << "  0\nSECTION\n"
         << "  2\nTABLES\n";

  // LTYPE table
  output << "  0\nTABLE\n"
         << "  2\nLTYPE\n"
         << " 70\n1\n"
         << "  0\nLTYPE\n"
         << "  2\nCONTINUOUS\n"
         << " 70\n64\n"
         << "  3\nSolid line\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.000000\n"
         << "  0\nENDTAB\n";

  // LAYER table
  output << "  0\nTABLE\n"
         << "  2\nLAYER\n"
         << " 70\n1\n"
         << "  0\nLAYER\n"
         << "  2\n0\n"
         << " 70\n64\n"
         << " 62\n7\n"
         << "  6\nCONTINUOUS\n"
         << "  0\nENDTAB\n";

  // STYLE table
  output << "  0\nTABLE\n"
         << "  2\nSTYLE\n"
         << " 70\n0\n"
         << "  0\nENDTAB\n";

  output << "  0\nENDSEC\n";

  // BLOCKS section (empty)
  output << "  0\nSECTION\n"
         << "  2\nBLOCKS\n"
         << "  0\nENDSEC\n";
}

static void export_dxf_header_R12(std::ostream& output, double xMin, double yMin, double xMax,
                                  double yMax, int handseed)
{
  output << "999\n"
         << "DXF from OpenSCAD (R12)\n"

         << "  0\nSECTION\n"
         << "  2\nHEADER\n"

         << "  9\n$ACADVER\n"
         << "  1\nAC1009\n"

         << "  9\n$DWGCODEPAGE\n"
         << "  3\nansi_1252\n"

         << "  9\n$INSBASE\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"

         << "  9\n$EXTMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$EXTMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  9\n$LIMMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$LIMMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  9\n$HANDLING\n"
         << " 70\n1\n"

         << "  9\n$HANDSEED\n"
         << "  5\n";
  emit_hexval(output, handseed);

  output << "  0\nENDSEC\n";

  // TABLES section
  output << "  0\nSECTION\n"
         << "  2\nTABLES\n";

  // LTYPE table
  output << "  0\nTABLE\n"
         << "  2\nLTYPE\n";
  emit_handle(output, H_LTYPE_TABLE_R12);
  output << " 70\n1\n"
         << "  0\nLTYPE\n";
  emit_handle(output, H_LTYPE_CONT_R12);
  output << "  2\nCONTINUOUS\n"
         << " 70\n64\n"
         << "  3\nSolid line\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.000000\n"
         << "  0\nENDTAB\n";

  // LAYER table
  output << "  0\nTABLE\n"
         << "  2\nLAYER\n";
  emit_handle(output, H_LAYER_TABLE_R12);
  output << " 70\n1\n"
         << "  0\nLAYER\n";
  emit_handle(output, H_LAYER_0_R12);
  output << "  2\n0\n"
         << " 70\n64\n"
         << " 62\n7\n"
         << "  6\nCONTINUOUS\n"
         << "  0\nENDTAB\n";

  // STYLE table
  output << "  0\nTABLE\n"
         << "  2\nSTYLE\n";
  emit_handle(output, H_STYLE_TABLE_R12);
  output << " 70\n0\n"
         << "  0\nENDTAB\n";

  output << "  0\nENDSEC\n";

  // BLOCKS section (empty)
  output << "  0\nSECTION\n"
         << "  2\nBLOCKS\n"
         << "  0\nENDSEC\n";
}

static void export_dxf_header_R14(std::ostream& output, double xMin, double yMin, double xMax,
                                  double yMax, int handseed, int insunits, int lunits)
{
  output << "999\n"
         << "DXF from OpenSCAD (R14)\n"

         << "  0\nSECTION\n"
         << "  2\nHEADER\n"

         << "  9\n$ACADVER\n"
         << "  1\nAC1014\n"

         << "  9\n$ACADMAINTVER\n"  // first variable in R14 spec; value 0 for base release
         << " 70\n0\n"

         << "  9\n$DWGCODEPAGE\n"
         << "  3\nansi_1252\n"

         << "  9\n$INSBASE\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"

         << "  9\n$EXTMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"
         << " 30\n0.0\n"

         << "  9\n$EXTMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"
         << " 30\n0.0\n"

         << "  9\n$LIMMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$LIMMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax << "\n"

         << "  9\n$HANDSEED\n"
         << "  5\n";
  emit_hexval(output, handseed);

  output << "  9\n$MEASUREMENT\n"
         << " 70\n1\n"
         << "  9\n$INSUNITS\n"
         << " 70\n"
         << insunits << "\n"
         << "  9\n$LUNITS\n"
         << " 70\n"
         << lunits << "\n";

  output << "  0\nENDSEC\n";

  // CLASSES section (required by R14, empty is valid)
  output << "  0\nSECTION\n"
         << "  2\nCLASSES\n"
         << "  0\nENDSEC\n";

  // ---------------------------------------------------------------
  // TABLES section
  //
  // Canonical AutoCAD R14 table order (Autodesk readers are
  // order-sensitive):
  //   VPORT, LTYPE, LAYER, STYLE, VIEW, UCS, APPID, DIMSTYLE,
  //   BLOCK_RECORD
  // ---------------------------------------------------------------
  output << "  0\nSECTION\n"
         << "  2\nTABLES\n";

  // VPORT table — must be first; *ACTIVE entry required by Autodesk viewers
  output << "  0\nTABLE\n"
         << "  2\nVPORT\n";
  emit_handle(output, H_VPORT_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n1\n"
         << "  0\nVPORT\n";
  emit_handle(output, H_VPORT_ACTIVE_R14);
  emit_owner(output, H_VPORT_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbViewportTableRecord\n"
         << "  2\n*ACTIVE\n"
         << " 70\n0\n"
         // lower-left corner of viewport
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         // upper-right corner of viewport
         << " 11\n1.0\n"
         << " 21\n1.0\n"
         // view centre point (midpoint of extents)
         << " 12\n"
         << (xMin + xMax) / 2.0 << "\n"
         << " 22\n"
         << (yMin + yMax) / 2.0
         << "\n"
         // snap base, snap spacing, grid spacing
         << " 13\n0.0\n"
         << " 23\n0.0\n"
         << " 14\n10.0\n"
         << " 24\n10.0\n"
         << " 15\n10.0\n"
         << " 25\n10.0\n"
         // view direction (WCS normal = +Z)
         << " 16\n0.0\n"
         << " 26\n0.0\n"
         << " 36\n1.0\n"
         // view target
         << " 17\n0.0\n"
         << " 27\n0.0\n"
         << " 37\n0.0\n"
         // view height = full Y extent, aspect ratio = X/Y
         << " 40\n"
         << (yMax - yMin) << "\n"
         << " 41\n"
         << (xMax - xMin) / (yMax - yMin)
         << "\n"
         // lens length, front/back clipping
         << " 42\n50.0\n"
         << " 43\n0.0\n"
         << " 44\n0.0\n"
         // snap/grid angles, twist
         << " 50\n0.0\n"
         << " 51\n0.0\n"
         // flags: 0=none; circle zoom pct; fast zoom; icon; snap; grid
         << " 71\n0\n"
         << " 72\n100\n"
         << " 73\n1\n"
         << " 74\n3\n"
         << " 75\n0\n"
         << " 76\n1\n"
         << " 77\n0\n"
         << " 78\n0\n"
         << "  0\nENDTAB\n";

  // LTYPE table — ByBlock and ByLayer entries are required by strict readers
  output << "  0\nTABLE\n"
         << "  2\nLTYPE\n";
  emit_handle(output, H_LTYPE_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n3\n"  // 3 entries: ByBlock, ByLayer, Continuous
         << "  0\nLTYPE\n";
  emit_handle(output, H_LTYPE_BYBLOCK_R14);
  emit_owner(output, H_LTYPE_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbLinetypeTableRecord\n"
         << "  2\nByBlock\n"
         << " 70\n0\n"
         << "  3\n\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.0\n"
         << "  0\nLTYPE\n";
  emit_handle(output, H_LTYPE_BYLAYER_R14);
  emit_owner(output, H_LTYPE_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbLinetypeTableRecord\n"
         << "  2\nByLayer\n"
         << " 70\n0\n"
         << "  3\n\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.0\n"
         << "  0\nLTYPE\n";
  emit_handle(output, H_LTYPE_CONT_R14);
  emit_owner(output, H_LTYPE_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbLinetypeTableRecord\n"
         << "  2\nCONTINUOUS\n"
         << " 70\n64\n"
         << "  3\nSolid line\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.0\n"
         << "  0\nENDTAB\n";

  // LAYER table — group 370 (lineweight) and 390 (plot style) required by R14
  output << "  0\nTABLE\n"
         << "  2\nLAYER\n";
  emit_handle(output, H_LAYER_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n1\n"
         << "  0\nLAYER\n";
  emit_handle(output, H_LAYER_0_R14);
  emit_owner(output, H_LAYER_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbLayerTableRecord\n"
         << "  2\n0\n"
         << " 70\n0\n"
         << " 62\n7\n"
         << "  6\nCONTINUOUS\n"
         << "370\n-3\n"  // lineweight: -3 = ByLayer default
         << "390\n0\n"   // plot style handle (0 = default/none)
         << "  0\nENDTAB\n";

  // STYLE table — "Standard" text style entry required
  output << "  0\nTABLE\n"
         << "  2\nSTYLE\n";
  emit_handle(output, H_STYLE_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n1\n"
         << "  0\nSTYLE\n";
  emit_handle(output, H_STYLE_STD_R14);
  emit_owner(output, H_STYLE_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbTextStyleTableRecord\n"
         << "  2\nStandard\n"
         << " 70\n0\n"
         << " 40\n0.0\n"
         << " 41\n1.0\n"
         << " 50\n0.0\n"
         << " 71\n0\n"
         << " 42\n2.5\n"
         << "  3\ntxt\n"
         << "  4\n\n"
         << "  0\nENDTAB\n";

  // VIEW table (empty, but required by strict R14 readers)
  output << "  0\nTABLE\n"
         << "  2\nVIEW\n";
  emit_handle(output, H_VIEW_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n0\n"
         << "  0\nENDTAB\n";

  // UCS table (empty, but required by strict R14 readers)
  output << "  0\nTABLE\n"
         << "  2\nUCS\n";
  emit_handle(output, H_UCS_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n0\n"
         << "  0\nENDTAB\n";

  // APPID table — ACAD entry is required by Autodesk readers
  output << "  0\nTABLE\n"
         << "  2\nAPPID\n";
  emit_handle(output, H_APPID_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n1\n"
         << "  0\nAPPID\n";
  emit_handle(output, H_APPID_ACAD_R14);
  emit_owner(output, H_APPID_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbRegAppTableRecord\n"
         << "  2\nACAD\n"
         << " 70\n0\n"
         << "  0\nENDTAB\n";

  // DIMSTYLE table — "Standard" entry required; handle uses group 105 per spec
  output << "  0\nTABLE\n"
         << "  2\nDIMSTYLE\n";
  emit_handle(output, H_DIMSTYLE_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n1\n"
         << "  0\nDIMSTYLE\n"
         << "105\n";  // DIMSTYLE entries use group 105, not 5, per R14 spec
  emit_hexval(output, H_DIMSTYLE_STD_R14);
  emit_owner(output, H_DIMSTYLE_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbDimStyleTableRecord\n"
         << "  2\nStandard\n"
         << " 70\n0\n"
         << "  0\nENDTAB\n";

  // BLOCK_RECORD table (required by R14)
  output << "  0\nTABLE\n"
         << "  2\nBLOCK_RECORD\n";
  emit_handle(output, H_BLOCKREC_TABLE_R14);
  emit_owner(output, 0);
  output << "100\nAcDbSymbolTable\n"
         << " 70\n2\n"
         << "  0\nBLOCK_RECORD\n";
  emit_handle(output, H_BLOCKREC_MODEL_R14);
  emit_owner(output, H_BLOCKREC_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbBlockTableRecord\n"
         << "  2\n*Model_Space\n"
         << "  0\nBLOCK_RECORD\n";
  emit_handle(output, H_BLOCKREC_PAPER_R14);
  emit_owner(output, H_BLOCKREC_TABLE_R14);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbBlockTableRecord\n"
         << "  2\n*Paper_Space\n"
         << "  0\nENDTAB\n";

  output << "  0\nENDSEC\n";

  // BLOCKS section (*Model_Space and *Paper_Space required by R14)
  output << "  0\nSECTION\n"
         << "  2\nBLOCKS\n"
         << "  0\nBLOCK\n";
  emit_handle(output, H_BLOCK_MODEL_R14);
  emit_owner(output, H_BLOCKREC_MODEL_R14);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockBegin\n"
         << "  2\n*Model_Space\n"
         << " 70\n0\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"
         << "  3\n*Model_Space\n"
         << "  1\n\n"
         << "  0\nENDBLK\n";
  emit_handle(output, H_ENDBLK_MODEL_R14);
  emit_owner(output, H_BLOCKREC_MODEL_R14);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockEnd\n"
         << "  0\nBLOCK\n";
  emit_handle(output, H_BLOCK_PAPER_R14);
  emit_owner(output, H_BLOCKREC_PAPER_R14);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockBegin\n"
         << "  2\n*Paper_Space\n"
         << " 70\n0\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"
         << "  3\n*Paper_Space\n"
         << "  1\n\n"
         << "  0\nENDBLK\n";
  emit_handle(output, H_ENDBLK_PAPER_R14);
  emit_owner(output, H_BLOCKREC_PAPER_R14);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockEnd\n"
         << "  0\nENDSEC\n";
}

static void export_dxf_objects_R14(std::ostream& output)
{
  output << "  0\nSECTION\n"
         << "  2\nOBJECTS\n"
         << "  0\nDICTIONARY\n";
  emit_handle(output, H_DICT_ROOT_R14);
  emit_owner(output, 0);  // owner = 0 (implicit drawing database root object)
  output << "100\nAcDbDictionary\n"
         << "281\n1\n"
         << "  3\nACAD_GROUP\n"
         << "350\n";
  emit_hexval(output, H_DICT_GROUP_R14);

  output << "  0\nDICTIONARY\n";
  emit_handle(output, H_DICT_GROUP_R14);
  emit_owner(output, H_DICT_ROOT_R14);
  output << "100\nAcDbDictionary\n"
         << "281\n1\n"
         << "  0\nENDSEC\n";
}

// =====================================================================
// ENTITY EXPORT - MAIN FUNCTION
// =====================================================================
//
// Exports a Polygon2d to DXF. Version controls entity format:
//
//   Legacy:  LWPOLYLINE always (original OpenSCAD behaviour, no handles)
//   R10:     POLYLINE/VERTEX/SEQEND (spec-correct, no handles)
//   R12:     POLYLINE/VERTEX/SEQEND (spec-correct, with handles)
//   R14:     LWPOLYLINE (spec-correct R14+ entity, with handles)
//
// POINT (1 vertex) and LINE (2 vertices) are handled the same in all
// modes Ã¢â‚¬â€ they are valid in all DXF versions.
//
// SEQEND in R12 also receives a handle. The handseed for R12 is
// calculated as:
//   H_ENT_START_R12 + sum over outlines of (2 + vertex_count)
// because each outline consumes: 1 POLYLINE handle + N VERTEX handles
// + 1 SEQEND handle.
// =====================================================================

static void export_dxf(const Polygon2d& poly, std::ostream& output, DxfVersion version)
{
  setlocale(LC_NUMERIC, "C");  // Force "." decimal separator

  // ---------------------------------------------------------------
  // Calculate bounding box
  // ---------------------------------------------------------------
  double xMin, yMin, xMax, yMax;
  xMin = yMin = std::numeric_limits<double>::max();
  xMax = yMax = std::numeric_limits<double>::min();

  for (const auto& o : poly.outlines()) {
    for (const auto& p : o.vertices) {
      if (xMin > p[0]) xMin = p[0];
      if (xMax < p[0]) xMax = p[0];
      if (yMin > p[1]) yMin = p[1];
      if (yMax < p[1]) yMax = p[1];
    }
  }

  // ---------------------------------------------------------------
  // Calculate handseed for R12 and R14
  //
  // R14: one handle per entity (LWPOLYLINE, POINT, LINE)
  // R12: POLYLINE=1, VERTEX=N, SEQEND=1 per outline  Ã¢â€ â€™  (2+N) per outline
  //      POINT and LINE outlines consume 1 handle each (no VERTEX/SEQEND)
  // ---------------------------------------------------------------
  int handseed_r12 = H_ENT_START_R12;
  int handseed_r14 = H_ENT_START_R14;

  for (const auto& o : poly.outlines()) {
    const int n = static_cast<int>(o.vertices.size());
    if (n <= 2) {
      // POINT or LINE: 1 handle in both R12 and R14
      handseed_r12 += 1;
      handseed_r14 += 1;
    } else {
      // R12: POLYLINE(1) + VERTEX(n) + SEQEND(1) = n+2
      handseed_r12 += n + 2;
      // R14: LWPOLYLINE(1)
      handseed_r14 += 1;
    }
  }

  // ---------------------------------------------------------------
  // Write header (version-specific)
  // ---------------------------------------------------------------
  switch (version) {
  case DxfVersion::Legacy:
    // Legacy uses its own header that exactly reproduces original OpenSCAD
    // output, including the $LINMIN/$LINMAX typos and plain comment string.
    export_dxf_header_Legacy(output, xMin, yMin, xMax, yMax);
    break;
  case DxfVersion::R10: export_dxf_header_R10(output, xMin, yMin, xMax, yMax); break;
  case DxfVersion::R12: export_dxf_header_R12(output, xMin, yMin, xMax, yMax, handseed_r12); break;
  case DxfVersion::R14:
    export_dxf_header_R14(output, xMin, yMin, xMax, yMax, handseed_r14, 4, 2);
    //                                                                    ^  ^
    //                                                          insunits=4(mm) lunits=2(decimal)
    break;
  }

  // ---------------------------------------------------------------
  // ENTITIES SECTION
  // ---------------------------------------------------------------
  output << "  0\nSECTION\n"
         << "  2\nENTITIES\n";

  int ent_handle = (version == DxfVersion::R14)   ? H_ENT_START_R14
                   : (version == DxfVersion::R12) ? H_ENT_START_R12
                                                  : 0;

  for (const auto& o : poly.outlines()) {
    const int n = static_cast<int>(o.vertices.size());

    if (n == 1) {
      // -----------------------------------------------------------
      // POINT entity Ã¢â‚¬â€ single vertex (valid in all versions)
      // -----------------------------------------------------------
      const Vector2d& p = o.vertices[0];
      output << "  0\nPOINT\n";
      if (version == DxfVersion::R12 || version == DxfVersion::R14) {
        emit_handle(output, ent_handle++);
      }
      // Legacy and R14 both emit subclass markers Ã¢â‚¬" the original OpenSCAD
      // code always emitted these for all entity types.
      if (version == DxfVersion::Legacy || version == DxfVersion::R14) {
        output << "100\nAcDbEntity\n";
      }
      output << "  8\n0\n";
      if (version == DxfVersion::R14) {
        output << "  6\nByLayer\n"
               << " 62\n256\n";
      }
      if (version == DxfVersion::Legacy || version == DxfVersion::R14) {
        output << "100\nAcDbPoint\n";
      }
      output << " 10\n"
             << p[0] << "\n"
             << " 20\n"
             << p[1] << "\n";

    } else if (n == 2) {
      // -----------------------------------------------------------
      // LINE entity Ã¢â‚¬â€ two vertices (valid in all versions)
      // -----------------------------------------------------------
      const Vector2d& p1 = o.vertices[0];
      const Vector2d& p2 = o.vertices[1];
      output << "  0\nLINE\n";
      if (version == DxfVersion::R12 || version == DxfVersion::R14) {
        emit_handle(output, ent_handle++);
      }
      // Legacy and R14 both emit subclass markers Ã¢â‚¬" the original OpenSCAD
      // code always emitted these for all entity types.
      if (version == DxfVersion::Legacy || version == DxfVersion::R14) {
        output << "100\nAcDbEntity\n";
      }
      output << "  8\n0\n";
      if (version == DxfVersion::R14) {
        output << "  6\nByLayer\n"
               << " 62\n256\n";
      }
      if (version == DxfVersion::Legacy || version == DxfVersion::R14) {
        output << "100\nAcDbLine\n";
      }
      output << " 10\n"
             << p1[0] << "\n"
             << " 20\n"
             << p1[1] << "\n"
             << " 11\n"
             << p2[0] << "\n"
             << " 21\n"
             << p2[1] << "\n";

    } else {
      // -----------------------------------------------------------
      // Polyline entity Ã¢â‚¬â€ three or more vertices (most common)
      //
      // Legacy/R14: LWPOLYLINE (single compact record)
      // R10/R12:    POLYLINE + VERTEX*N + SEQEND (pre-R14 spec)
      // -----------------------------------------------------------
      if (version == DxfVersion::Legacy || version == DxfVersion::R14) {
        // LWPOLYLINE Ã¢â‚¬â€ compact single-record form
        output << "  0\nLWPOLYLINE\n";
        if (version == DxfVersion::R14) {
          emit_handle(output, ent_handle++);
        }
        // Both Legacy and R14 emit subclass markers Ã¢â‚¬â€ the original OpenSCAD
        // code always emitted these even though they are technically R14-only.
        output << "100\nAcDbEntity\n";
        output << "  8\n0\n";
        if (version == DxfVersion::R14) {
          output << "  6\nByLayer\n"
                 << " 62\n256\n";
        }
        output << "100\nAcDbPolyline\n";
        output << " 90\n"
               << n << "\n"
               << " 70\n1\n";  // closed polyline flag
        if (version == DxfVersion::R14) {
          output << " 43\n0\n";  // constant width = 0
        }
        for (const auto& p : o.vertices) {
          output << " 10\n"
                 << p[0] << "\n"
                 << " 20\n"
                 << p[1] << "\n";
        }

      } else {
        // POLYLINE / VERTEX / SEQEND Ã¢â‚¬â€ spec-correct for R10 and R12
        output << "  0\nPOLYLINE\n";
        if (version == DxfVersion::R12) {
          emit_handle(output, ent_handle++);
        }
        output << "  8\n0\n"
               << " 66\n1\n"   // vertices-follow flag
               << " 70\n1\n";  // closed polyline flag

        for (const auto& p : o.vertices) {
          output << "  0\nVERTEX\n";
          if (version == DxfVersion::R12) {
            emit_handle(output, ent_handle++);
          }
          output << "  8\n0\n"
                 << " 10\n"
                 << p[0] << "\n"
                 << " 20\n"
                 << p[1] << "\n";
        }

        output << "  0\nSEQEND\n";
        if (version == DxfVersion::R12) {
          emit_handle(output, ent_handle++);
        }
      }
    }
  }

  output << "  0\nENDSEC\n";

  // OBJECTS section (R14 only)
  if (version == DxfVersion::R14) {
    export_dxf_objects_R14(output);
  }

  output << "  0\nEOF\n";

  setlocale(LC_NUMERIC, "");  // Restore locale
}

// =====================================================================
// PUBLIC API FUNCTIONS
// =====================================================================
//
// Entry points called by OpenSCAD's export system.
// Dispatch to appropriate handler based on geometry type.
//
// Supported geometry types:
//   - Polygon2d:    2D polygon (the primary use case)
//   - GeometryList: Recursively export each child
//   - PolySet:      Not supported (3D mesh, would need projection)
// =====================================================================

void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output, DxfVersion version)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      export_dxf(item.second, output, version);
    }
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    export_dxf(*poly, output, version);
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) {
    assert(false && "Unsupported file format");
  } else {
    assert(false && "Export as DXF for this geometry type is not supported");
  }
}

// Overload without version parameter Ã¢â‚¬â€ defaults to Legacy (original behaviour)
void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  export_dxf(geom, output, DxfVersion::Legacy);
}

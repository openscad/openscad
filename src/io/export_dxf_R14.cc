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

// ---------------------------------------------------------------------
// R14 (AC1014) compatible DXF exporter — full object model.
//
// This is a drop-in replacement for src/io/export_dxf.cc.  It emits
// AC1014 instead of AC1006 and implements the R14 object model that
// the upstream exporter lacks entirely.
//
// What changed from upstream and why:
//
//   Version string: AC1006 -> AC1014
//       R14 is the minimum version that carries an object model.
//       Declaring AC1014 tells readers to expect handles, owner
//       references, CLASSES, OBJECTS, and BLOCK_RECORD — all of
//       which we now emit.
//
//   Handles (group 5) on every record
//       R14 requires every table entry, block definition, block
//       record, entity, and object to carry a unique hex handle.
//       Handles are allocated sequentially starting at 1.  Layout:
//
//         1   TABLE LTYPE
//         2   LTYPE Continuous
//         3   TABLE LAYER
//         4   LAYER "0"              owner -> 3
//         5   TABLE STYLE
//         6   TABLE BLOCK_RECORD
//         7   BLOCK_RECORD *Model_Space
//         8   BLOCK_RECORD *Paper_Space
//         9   BLOCK *Model_Space
//         A   ENDBLK (Model)
//         B   BLOCK *Paper_Space
//         C   ENDBLK (Paper)
//         D   DICTIONARY (root)      softptr -> E
//         E   DICTIONARY (group)     owner -> D
//         F+  entities               one per outline, sequential
//
//       Owner references (group 330) are emitted only where the
//       reference file has them: LAYER "0" -> its TABLE, and the
//       ACAD_GROUP dictionary -> root dictionary.  Tables, blocks,
//       block records, and entities carry handles but no owners,
//       matching the reference exactly.
//
//   $HANDSEED header variable
//       Declares the next available handle value so readers know
//       the safe allocation floor.  Uses group code 5 (it is a
//       handle value).  Set to 0xF + outline_count.  Requires a
//       pre-pass over outlines to count them — we piggyback on the
//       existing bounding-box pre-pass.
//
//   $DWGCODEPAGE header variable
//       Declares character encoding.  Set to ansi_1252.
//
//   Unit variables: $MEASUREMENT and $LUNITS
//       $MEASUREMENT (group 70): 0 = imperial, 1 = metric.  R14-era
//       toggle; readers that don't understand $LUNITS fall back here.
//       $LUNITS (group 70): specifies precise linear units.  Value 4
//       = millimeters is the default; callers can pass a different
//       value (1=inches, 5=centimeters, 7=meters, etc.).  Both are
//       emitted so the file is correct for R14 and R2000+ readers.
//
//   Root DICTIONARY owner -> handle 0
//       Handle 0 is the implicit "drawing object" in the AutoCAD
//       object model — it is never written to the file, but the root
//       DICTIONARY's owner (group 330) must reference it.  Omitting
//       this causes strict importers to flag "invalid owner handle"
//       and silently add the missing reference.
//
//   CLASSES section
//       Required section marker in R14+.  Empty body — no extended
//       entity types in a geometry-only export.
//
//   BLOCK_RECORD table
//       Two entries (*Model_Space, *Paper_Space) that formally
//       register the block names in the object model.  Companion
//       to the BLOCKS section definitions.
//
//   Subclass markers on table entries and blocks
//       LTYPE entries:  100/AcDbSymbolTableRecord + 100/AcDbLinetypeTableRecord
//       LAYER entries:  100/AcDbSymbolTableRecord + 100/AcDbLayerTableRecord
//       BLOCK_RECORD:   100/AcDbSymbolTableRecord + 100/AcDbBlockTableRecord
//       BLOCK:          100/AcDbEntity + 100/AcDbBlockBegin
//       ENDBLK:         100/AcDbEntity + 100/AcDbBlockEnd
//
//   Explicit entity defaults (group 6, 62, 43)
//       6/ByLayer, 62/256, 43/0 written on every entity.  All are
//       defaults, but strict R14 readers expect them stated.
//
//   OBJECTS section
//       Minimal root dictionary with one ACAD_GROUP child.
//       Required by R14; without it readers that expect an object
//       database reject the file.  Emitted after ENTITIES.
//
// Unchanged from upstream: POINT for 1-vertex outlines, LINE for
// 2-vertex, LWPOLYLINE for 3+, the 999 comment, locale handling,
// and the GeometryList recursive dispatch.
// ---------------------------------------------------------------------

#include "io/export.h"

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

// ---------------------------------------------------------------------
// Fixed handle assignments.  Everything up to and including the two
// OBJECTS dictionaries is constant regardless of geometry content.
// Entities are allocated sequentially starting at HANDLE_ENT_START.
// ---------------------------------------------------------------------
static constexpr int H_LTYPE_TABLE = 0x1;
static constexpr int H_LTYPE_CONT = 0x2;
static constexpr int H_LAYER_TABLE = 0x3;
static constexpr int H_LAYER_0 = 0x4;
static constexpr int H_STYLE_TABLE = 0x5;
static constexpr int H_BLOCKREC_TABLE = 0x6;
static constexpr int H_BLOCKREC_MODEL = 0x7;
static constexpr int H_BLOCKREC_PAPER = 0x8;
static constexpr int H_BLOCK_MODEL = 0x9;
static constexpr int H_ENDBLK_MODEL = 0xA;
static constexpr int H_BLOCK_PAPER = 0xB;
static constexpr int H_ENDBLK_PAPER = 0xC;
static constexpr int H_DICT_ROOT = 0xD;
static constexpr int H_DICT_GROUP = 0xE;
static constexpr int H_ENT_START = 0xF;  // first entity

// Emit group 5 (handle) as uppercase hex.
static void h(std::ostream& o, int handle)
{
  o << "  5\n" << std::uppercase << std::hex << handle << std::dec << "\n";
}

// Emit group 330 (owner handle) as uppercase hex.
static void own(std::ostream& o, int handle)
{
  o << "330\n" << std::uppercase << std::hex << handle << std::dec << "\n";
}

// Emit a bare hex value (no group code) — used for 350 soft-pointer
// and $HANDSEED value lines.
static void hexval(std::ostream& o, int handle)
{
  o << std::uppercase << std::hex << handle << std::dec << "\n";
}

static void export_dxf_header(std::ostream& output, double xMin, double yMin, double xMax, double yMax,
                              int handseed, int lunits)
{
  output << "999\n"
         << "DXF from OpenSCAD\n";

  // ---- HEADER section --------------------------------------------------
  output << "  0\nSECTION\n"
         << "  2\nHEADER\n"

         << "  9\n$ACADVER\n"
         << "  1\nAC1014\n"

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

         << "  9\n$LINMIN\n"
         << " 10\n"
         << xMin << "\n"
         << " 20\n"
         << yMin << "\n"

         << "  9\n$LINMAX\n"
         << " 10\n"
         << xMax << "\n"
         << " 20\n"
         << yMax
         << "\n"

         // $HANDSEED — uses group code 5 (it is a handle value).
         << "  9\n$HANDSEED\n"
         << "  5\n";
  hexval(output, handseed);

  // $MEASUREMENT: 1 = metric.  R14-era metric/imperial toggle.
  // $LUNITS: precise linear unit.  Default 4 = millimeters; caller
  // can override (1=inches, 5=centimeters, 7=meters, etc.).
  output << "  9\n$MEASUREMENT\n"
         << " 70\n1\n"
         << "  9\n$LUNITS\n"
         << " 70\n"
         << lunits << "\n";

  output << "  0\nENDSEC\n";

  // ---- CLASSES section -------------------------------------------------
  // Required in R14+.  Empty — no extended entity types.
  output << "  0\nSECTION\n"
         << "  2\nCLASSES\n"
         << "  0\nENDSEC\n";

  // ---- TABLES section --------------------------------------------------
  output << "  0\nSECTION\n"
         << "  2\nTABLES\n";

  // --- LTYPE -----------------------------------------------------------
  output << "  0\nTABLE\n"
         << "  2\nLTYPE\n";
  h(output, H_LTYPE_TABLE);
  output << " 70\n1\n"

         << "  0\nLTYPE\n";
  h(output, H_LTYPE_CONT);
  // No owner on base LTYPEs (matches reference).
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbLinetypeTableRecord\n"
         << "  2\nCONTINUOUS\n"
         << " 70\n64\n"
         << "  3\nSolid line\n"
         << " 72\n65\n"
         << " 73\n0\n"
         << " 40\n0.000000\n"

         << "  0\nENDTAB\n";

  // --- LAYER -----------------------------------------------------------
  output << "  0\nTABLE\n"
         << "  2\nLAYER\n";
  h(output, H_LAYER_TABLE);
  output << " 70\n6\n"

         << "  0\nLAYER\n";
  h(output, H_LAYER_0);
  own(output, H_LAYER_TABLE);  // LAYER "0" owns -> LAYER table
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbLayerTableRecord\n"
         << "  2\n0\n"  // layer name
         << " 70\n0\n"
         << " 62\n7\n"  // color
         << "  6\nCONTINUOUS\n"

         << "  0\nENDTAB\n";

  // --- STYLE -----------------------------------------------------------
  // Empty stub, preserved from upstream.
  output << "  0\nTABLE\n"
         << "  2\nSTYLE\n";
  h(output, H_STYLE_TABLE);
  output << " 70\n0\n"
         << "  0\nENDTAB\n";

  // --- BLOCK_RECORD ----------------------------------------------------
  output << "  0\nTABLE\n"
         << "  2\nBLOCK_RECORD\n";
  h(output, H_BLOCKREC_TABLE);
  output << " 70\n2\n"  // 2 entries

         << "  0\nBLOCK_RECORD\n";
  h(output, H_BLOCKREC_MODEL);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbBlockTableRecord\n"
         << "  2\n*Model_Space\n"

         << "  0\nBLOCK_RECORD\n";
  h(output, H_BLOCKREC_PAPER);
  output << "100\nAcDbSymbolTableRecord\n"
         << "100\nAcDbBlockTableRecord\n"
         << "  2\n*Paper_Space\n"

         << "  0\nENDTAB\n";

  output << "  0\nENDSEC\n";

  // ---- BLOCKS section --------------------------------------------------
  output << "  0\nSECTION\n"
         << "  2\nBLOCKS\n"

         // *Model_Space BLOCK
         << "  0\nBLOCK\n";
  h(output, H_BLOCK_MODEL);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockBegin\n"
         << "  2\n*Model_Space\n"
         << " 70\n0\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"
         << "  3\n*Model_Space\n"

         // *Model_Space ENDBLK
         << "  0\nENDBLK\n";
  h(output, H_ENDBLK_MODEL);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockEnd\n"

         // *Paper_Space BLOCK
         << "  0\nBLOCK\n";
  h(output, H_BLOCK_PAPER);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockBegin\n"
         << "  2\n*Paper_Space\n"
         << " 70\n0\n"
         << " 10\n0.0\n"
         << " 20\n0.0\n"
         << " 30\n0.0\n"
         << "  3\n*Paper_Space\n"

         // *Paper_Space ENDBLK
         << "  0\nENDBLK\n";
  h(output, H_ENDBLK_PAPER);
  output << "100\nAcDbEntity\n"
         << "  8\n0\n"
         << "100\nAcDbBlockEnd\n"

         << "  0\nENDSEC\n";
}

static void export_dxf_objects(std::ostream& output)
{
  // ---- OBJECTS section -------------------------------------------------
  // Minimal root dictionary + ACAD_GROUP child.  The root's owner
  // (group 330) points to handle 0 — the implicit "drawing object"
  // that is never written to the file but is the designated owner of
  // the root dictionary in the AutoCAD object model.  Omitting this
  // causes strict importers to flag an invalid owner handle.
  output << "  0\nSECTION\n"
         << "  2\nOBJECTS\n"

         // Root dictionary
         << "  0\nDICTIONARY\n";
  h(output, H_DICT_ROOT);
  own(output, 0);  // owner -> handle 0 (drawing object)
  output << "100\nAcDbDictionary\n"
         << "281\n1\n"           // hard-owner flag
         << "  3\nACAD_GROUP\n"  // key
         << "350\n";             // soft-pointer value follows
  hexval(output, H_DICT_GROUP);

  // ACAD_GROUP child dictionary (empty)
  output << "  0\nDICTIONARY\n";
  h(output, H_DICT_GROUP);
  own(output, H_DICT_ROOT);  // owner -> root
  output << "100\nAcDbDictionary\n"
         << "281\n1\n"

         << "  0\nENDSEC\n";
}

static void export_dxf(const Polygon2d& poly, std::ostream& output)
{
  setlocale(LC_NUMERIC, "C");  // Ensure radix is . (not ,) in output

  // --- Pre-pass: bounding box + outline count --------------------------
  // Both are needed before emission starts.  bbox -> HEADER extents.
  // outline count -> $HANDSEED (= H_ENT_START + count).
  double xMin, yMin, xMax, yMax;
  xMin = yMin = std::numeric_limits<double>::max();
  xMax = yMax = std::numeric_limits<double>::min();
  int outline_count = 0;
  for (const auto& o : poly.outlines()) {
    outline_count++;
    for (const auto& p : o.vertices) {
      if (xMin > p[0]) xMin = p[0];
      if (xMax < p[0]) xMax = p[0];
      if (yMin > p[1]) yMin = p[1];
      if (yMax < p[1]) yMax = p[1];
    }
  }

  int handseed = H_ENT_START + outline_count;
  export_dxf_header(output, xMin, yMin, xMax, yMax, handseed, 4);  // 4 = mm

  // ---- ENTITIES section ------------------------------------------------
  output << "  0\nSECTION\n"
         << "  2\nENTITIES\n";

  int ent_handle = H_ENT_START;
  for (const auto& o : poly.outlines()) {
    switch (o.vertices.size()) {
    case 1: {
      // POINT
      const Vector2d& p = o.vertices[0];
      output << "  0\nPOINT\n";
      h(output, ent_handle++);
      output << "100\nAcDbEntity\n"
             << "  8\n0\n"
             << "  6\nByLayer\n"
             << " 62\n256\n"
             << "100\nAcDbPoint\n"
             << " 10\n"
             << p[0] << "\n"
             << " 20\n"
             << p[1] << "\n";
    } break;
    case 2: {
      // LINE
      const Vector2d& p1 = o.vertices[0];
      const Vector2d& p2 = o.vertices[1];
      output << "  0\nLINE\n";
      h(output, ent_handle++);
      output << "100\nAcDbEntity\n"
             << "  8\n0\n"
             << "  6\nByLayer\n"
             << " 62\n256\n"
             << "100\nAcDbLine\n"
             << " 10\n"
             << p1[0] << "\n"
             << " 20\n"
             << p1[1] << "\n"
             << " 11\n"
             << p2[0] << "\n"
             << " 21\n"
             << p2[1] << "\n";
    } break;
    default:
      // LWPOLYLINE (closed)
      output << "  0\nLWPOLYLINE\n";
      h(output, ent_handle++);
      output << "100\nAcDbEntity\n"
             << "  8\n0\n"
             << "  6\nByLayer\n"
             << " 62\n256\n"
             << "100\nAcDbPolyline\n"
             << " 90\n"
             << o.vertices.size() << "\n"
             << " 70\n1\n"   // closed
             << " 43\n0\n";  // constant width
      for (const auto& p : o.vertices) {
        output << " 10\n"
               << p[0] << "\n"
               << " 20\n"
               << p[1] << "\n";
      }
      break;
    }
  }

  output << "  0\nENDSEC\n";

  // OBJECTS goes after ENTITIES per R14 section order.
  export_dxf_objects(output);

  output << "  0\nEOF\n";

  setlocale(LC_NUMERIC, "");  // restore default locale
}

void export_dxf(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      export_dxf(item.second, output);
    }
  } else if (const auto poly = std::dynamic_pointer_cast<const Polygon2d>(geom)) {
    export_dxf(*poly, output);
  } else if (std::dynamic_pointer_cast<const PolySet>(geom)) {  // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else {  // NOLINT(bugprone-branch-clone)
    assert(false && "Export as DXF for this geometry type is not supported");
  }
}

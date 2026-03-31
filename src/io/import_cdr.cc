/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
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
#include <memory>
#include <string>
#include <vector>
//
#include "core/CurveDiscretizer.h"
#include "geometry/Polygon2d.h"
#include <iostream>
#include "core/ColorUtil.h"

#include <libcdr/libcdr.h>
#include <librevenge/librevenge.h>
#include <algorithm>
#include <charconv>
#include <cstring>       // for std::strstr
#include <cmath>         // for std::pow
#include <system_error>  // for std::errc used with std::from_chars
#include <utils/printutils.h>
class CDRReader : public librevenge::RVNGDrawingInterface
{
public:
  CDRReader() : x(0.0), y(0.0), x1(0.0), y1(0.0), x2(0.0), y2(0.0), action() {}

  void startDocument(const librevenge::RVNGPropertyList& propList) override {}
  void endDocument() override {}

  void drawPath(const librevenge::RVNGPropertyList& propList) override
  {
    this->parseProp(0, propList, 0);
  }
  void drawGraphicObject(const librevenge::RVNGPropertyList& propList) override {}
  void setDocumentMetaData(const librevenge::RVNGPropertyList&) override {}
  void defineEmbeddedFont(const librevenge::RVNGPropertyList& propList) override {}
  void startPage(const librevenge::RVNGPropertyList& propList) override {}
  void startMasterPage(const librevenge::RVNGPropertyList& propList) override {}
  void setStyle(const librevenge::RVNGPropertyList& propList) override {}
  void startLayer(const librevenge::RVNGPropertyList& propList) override {}
  void endLayer() override {}
  void startEmbeddedGraphics(const librevenge::RVNGPropertyList& propList) override {}
  void endEmbeddedGraphics() override {}
  void openGroup(const librevenge::RVNGPropertyList& propList) override {}
  void closeGroup() override {}
  void drawRectangle(const librevenge::RVNGPropertyList& propList) override {}
  void drawEllipse(const librevenge::RVNGPropertyList& propList) override {}
  void drawPolygon(const librevenge::RVNGPropertyList& propList) override {}
  void drawPolyline(const librevenge::RVNGPropertyList& propList) override {}
  void drawConnector(const librevenge::RVNGPropertyList& propList) override {}
  void startTextObject(const librevenge::RVNGPropertyList& propList) override {}
  void endTextObject() override {}
  void startTableObject(const librevenge::RVNGPropertyList& propList) override {}
  void openTableRow(const librevenge::RVNGPropertyList& propList) override {}
  void closeTableRow() override {}
  void openTableCell(const librevenge::RVNGPropertyList& propList) override {}
  void closeTableCell() override {}
  void insertCoveredTableCell(const librevenge::RVNGPropertyList& propList) override {}
  void endTableObject() override {}
  void insertTab() override {}
  void insertSpace() override {}
  void insertText(const librevenge::RVNGString& text) override {}
  void insertLineBreak() override {}
  void insertField(const librevenge::RVNGPropertyList& propList) override {}
  void openOrderedListLevel(const librevenge::RVNGPropertyList& propList) override {}
  void openUnorderedListLevel(const librevenge::RVNGPropertyList& propList) override {}
  void closeOrderedListLevel() override {}
  void closeUnorderedListLevel() override {}
  void openListElement(const librevenge::RVNGPropertyList& propList) override {}
  void closeListElement() override {}
  void defineParagraphStyle(const librevenge::RVNGPropertyList& propList) override {}
  void openParagraph(const librevenge::RVNGPropertyList& propList) override {}
  void closeParagraph() override {}
  void defineCharacterStyle(const librevenge::RVNGPropertyList& propList) override {}
  void openSpan(const librevenge::RVNGPropertyList& propList) override {}
  void closeSpan() override {}
  void openLink(const librevenge::RVNGPropertyList& propList) override {}
  void closeLink() override {}
  void endPage() override {}
  void endMasterPage() override {}

  double parseCdrFloat(const std::string& s)
  {
    double value = std::stod(s);
    if (std::strstr(s.c_str(), "in") != nullptr) value = value * 25.4;
    if (std::strstr(s.c_str(), "cm") != nullptr) value = value * 10;

    return value;
  }
  void parseProp(int ident, const librevenge::RVNGPropertyList& propList, int mode)
  {
    // Ensure per-call parsing state starts from known values.
    x = 0.0;
    y = 0.0;
    x1 = 0.0;
    y1 = 0.0;
    x2 = 0.0;
    y2 = 0.0;
    action.clear();

    librevenge::RVNGPropertyList::Iter it(propList);
    Outline2d outl;
    outl.color = *OpenSCAD::parse_color("#f9d72c");

    for (it.rewind(); it.next();) {
      if (it.child() != nullptr) {
        std::string key = it.key();
        if (key == "svg:d" && ident == 0 && mode == 0) {
          const librevenge::RVNGPropertyListVector *vec = it.child();

          librevenge::RVNGPropertyListVector::Iter pit(*vec);

          for (pit.rewind(); pit.next();) {
            librevenge::RVNGPropertyList sublist = pit();
            this->parseProp(ident + 1, sublist, mode);
            if (action == "M") {
              if (outl.vertices.size() > 0) {
                if (outl.vertices.size() > 0) result.addOutline(outl);
                outl.vertices.clear();
              }
              outl.vertices.push_back(Vector2d(x, y));
            } else if (action == "L") {
              outl.vertices.push_back(Vector2d(x, y));
            } else if (action == "Z") {
              // close done implicitely as, PythonSCAD does not have open polygons
              if (outl.vertices.size() > 0) result.addOutline(outl);
              outl.vertices.clear();
            } else if (action == "C") {
              Vector2d p0 = outl.vertices[outl.vertices.size() - 1];
              Vector2d p1 = Vector2d(x1, y1);
              Vector2d p2 = Vector2d(x2, y2);
              Vector2d p3 = Vector2d(x, y);
              for (int i = 1; i <= 10; i++) {
                double t = i / 10.0;
                Vector2d pt = p0 * 1 * pow(1 - t, 3) + p1 * 3 * pow(1 - t, 2) * t +
                              p2 * 3 * (1 - t) * t * t + p3 * t * t * t;
                outl.vertices.push_back(pt);
              }
            } else std::cout << "Unknown action" << std::endl;
          }
          if (outl.vertices.size() > 0) result.addOutline(outl);
        }
      }
      const librevenge::RVNGProperty *prop = it();
      if (prop != nullptr) {
        if (prop->getStr() != nullptr) {
          std::string key = it.key();
          if (key == "svg:d")
            ;
          else if (key == "librevenge:path-action") action = prop->getStr().cstr();
          else if (key == "svg:x") x = parseCdrFloat(prop->getStr().cstr());
          else if (key == "svg:y") y = parseCdrFloat(prop->getStr().cstr());
          else if (key == "svg:x1") x1 = parseCdrFloat(prop->getStr().cstr());
          else if (key == "svg:y1") y1 = parseCdrFloat(prop->getStr().cstr());
          else if (key == "svg:x2") x2 = parseCdrFloat(prop->getStr().cstr());
          else if (key == "svg:y2") y2 = parseCdrFloat(prop->getStr().cstr());
          else {
            std::cout << "Key: " << it.key();
            std::cout << " = " << prop->getStr().cstr();
            std::cout << std::endl;
          }
        } else if (prop->getDouble()) std::cout << " = " << prop->getDouble();
        else if (prop->getInt()) std::cout << " = " << prop->getInt();
      }
    }
  }
  double x, y, x1, y1, x2, y2;
  std::string action;
  Polygon2d result;
};

std::unique_ptr<Polygon2d> import_cdr(CurveDiscretizer discretizer, const std::string& filename,
                                      const Location& loc)
{
  librevenge::RVNGFileStream input(filename.c_str());
  CDRReader reader;

  if (!libcdr::CDRDocument::parse(&input, &reader)) {
    std::cerr << "Parse failed\n";
    return std::make_unique<Polygon2d>();
  }

  return std::make_unique<Polygon2d>(reader.result);
}

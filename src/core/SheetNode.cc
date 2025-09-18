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

#include "core/SheetNode.h"

#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "core/Builtins.h"
#include "core/Children.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"
#include "lodepng/lodepng.h"

#include <algorithm>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>

std::unique_ptr<const Geometry> sheetCreateFuncGeometry(void *funcptr, double imin, double imax, double jmin, double jmax, double fs, bool ispan, bool jspan);

std::unique_ptr<const Geometry> SheetNode::createGeometry() const
{
#ifdef ENABLE_PYTHON
  return sheetCreateFuncGeometry(this->func, this->imin, this->imax, this->jmin, this->jmax, fs, this->ispan, this->jspan);
#endif
  PolySetBuilder builder(0, 0);
  return builder.build();
}

std::string SheetNode::toString() const
{
  std::ostringstream stream;

  stream << this->name() << "(func = " << rand()
         << ", fs = " << (this->fs)
         << ", imin = " << (this->imin)
         << ", imax = " << (this->imax)
         << ", jmin = " << (this->jmin)
         << ", jmax = " << (this->jmax) 
         << ", ispan = " << (this->ispan) 
         << ", jspan = " << (this->jspan) ;
  return stream.str();
}


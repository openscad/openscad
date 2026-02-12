/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2024 Clifford Wolf <clifford@clifford.at> and
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
#include <ostream>

#include "geometry/Geometry.h"
#include "io/export.h"
#include "utils/printutils.h"

void export_3mf(const std::vector<struct Export3mfInfo>& infos, std::ostream& output, const ExportInfo&)
{
  LOG("Export to 3MF format was not enabled when building the application.");
}

void Export3mfPartInfo::writePropsFloat(void *pobj, const char *name, float f) const
{
  //	Lib3MF::PMeshObject  *obj = (Lib3MF::PMeshObject *) pobj;
  //	printf("Writing %s: %f\n",name, f);
  // void SetObjectLevelProperty(const Lib3MF_uint32 nUniqueResourceID, const Lib3MF_uint32 nPropertyID);
}
void Export3mfPartInfo::writePropsLong(void *pobj, const char *name, long l) const
{
  //	printf("Writing %s: %d\n",name, l);
}
void Export3mfPartInfo::writePropsString(void *pobj, const char *name, const char *val) const
{
  //	printf("Writing %s: %s\n",name, val);
}

void export_3mf(const std::vector<struct Export3mfPartInfo>& infos, std::ostream& output,
                const ExportInfo& exportInfo)
{
}

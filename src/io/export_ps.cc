/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2021      Konstantin Podsvirov <konstantin@podsvirov.pro>
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

#include "src/geometry/PolySet.h"
#include "src/geometry/cgal/cgalutils.h"
#include "src/geometry/PolySetUtils.h"
#include <unordered_map>
#include "src/utils/boost-utils.h"
#include <src/utils/hash.h>
#ifdef ENABLE_CAIRO
#include <cairo.h>
#include <cairo-pdf.h>
#endif

#include "export_foldable.h"


void output_ps(std::ostream &output, std::vector<sheetS> &sheets, const plotSettingsS & plot_s)
{
  double factor=72.0/25.4;
  output << "%!PS-Adobe-2.0\n";
  output << "%%Orientation: Portrait\n";
  output << "%%DocumentMedia: " << plot_s.paperformat << " " << plot_s.paperwidth*factor << " " << plot_s.paperheight * factor << "\n";
  output << "/Times-Roman findfont " << plot_s.lasche*2 << " scalefont setfont 0.1 setlinewidth\n";

  int pages=0;
  for(auto &sheet : sheets)
  {  
    // doppelte in lines loesche//n
    output << "%%Page: " << pages+1 << "\n";

//    printf("line size is %d\n",sheet.lines.size());	
    output << "0 0 0 setrgbcolor\n";
    double xofs = (plot_s.paperwidth-sheet.max[0]+sheet.min[0])/2.0-sheet.min[0];
    double yofs = (plot_s.paperheight-sheet.max[1]+sheet.min[1])/2.0-sheet.min[1];

    for(unsigned int i=0;i<sheet.lines.size();i++)
    {

      if(sheet.lines[i].dashed) output << "[2.5 2] 0 setdash\n";
      else output << "[3 0 ] 0 setdash\n";
      output << "newpath\n";
      output << (xofs+sheet.lines[i].p1[0])*factor << " " << (yofs+sheet.lines[i].p1[1])*factor << " moveto\n";
      output << (xofs+sheet.lines[i].p2[0])*factor << " " << (yofs+sheet.lines[i].p2[1])*factor << " lineto\n";
      output << "stroke\n";
    }
    for(unsigned int i=0;i<sheet.label.size();i++)
    {
      output << (xofs+sheet.label[i].pt[0])*factor << " " << (yofs+sheet.label[i].pt[1])*factor << " moveto\n";
      output << "gsave " << sheet.label[i].rot << " rotate ( " << sheet.label[i].text << " ) show grestore \n";
    }
    output << "10 10 moveto\n";
    std::string filename="a.ps"; // TODO weg
    output << "( " << filename << "/" << pages+1 << ") show \n";
    output << "showpage\n";
    pages++;
  }
  return;
}


void export_ps(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  std::shared_ptr<const PolySet> ps= PolySetUtils::getGeometryAsPolySet(geom);
  if(ps == nullptr) {
    printf("Dont have PolySet\n");	  
    return;
  }
  plotSettingsS plot_s;
  plot_s.lasche=10.0;
  plot_s.rand=5.0;
  plot_s.paperformat="A4";
  plot_s.bestend=0; // TODO besser
  if(strcmp(plot_s.paperformat, "A4") == 0) { plot_s.paperheight=298; plot_s.paperwidth=210; }
  else {plot_s.paperheight=420; plot_s.paperwidth=298;}

  std::vector<sheetS> sheets = fold_3d(ps, plot_s);
  output_ps(output, sheets,plot_s);
}



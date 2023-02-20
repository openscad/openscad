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

#include <QString>
//#include "PrintService.h"
#include "ExportPdfDialog.h"

ExportPdfDialog::ExportPdfDialog()
{
  setupUi(this);
}
// Getters

 
  double ExportPdfDialog::getGridSize() {
  	// return gridEnum2double[buttons2grid[gridButtonGroup->checkedId()]];
  	//  5 values are coded:  2, 2.5, 4, 5, 10; All are integer divisors of 20.
  	switch (gridButtonGroup->checkedId()) {
  		case -2:
			return 2.;
			break;
  		case -3:
			return 2.5;
			break;
  		case -4:
			return 4.;
			break;
		case -5:
			return 5.;
			break;
		case -6:
			return 10.;
			break;
		default:  // Always return a valid value.
		  	//LOG(error);
		  	return 5;
			break;

	}
  	
  }

  
  paperOrientations ExportPdfDialog::getOrientation() {
  	// return buttons2orientations[orientButtonGroup->checkedId()];
  	  	switch (orientButtonGroup->checkedId()) {
  		case -2:
			return paperOrientations::PORTRAIT;
			break;
  		case -3:
			return paperOrientations::LANDSCAPE;
			break;
  		case -4:
			return paperOrientations::AUTO;
			break;
		  default:  // Always return a valid value.
		  	//LOG(error);
			return paperOrientations::AUTO;
			break;

	}
 }
    		
  paperSizes ExportPdfDialog::getPaperSize() {
  	// return buttons2paper[sizeButtonGroup->checkedId()];
  	switch (sizeButtonGroup->checkedId()) {
  		case -2:
			return paperSizes::A4;
			break;
  		case -3:
			return paperSizes::A3;
			break;
  		case -4:
			return paperSizes::LETTER;
			break;
		  case -5:
			return paperSizes::LEGAL;
			break;
		  case -6:
			return paperSizes::TABLOID;
			break;
		  default:  // Always return a valid value.
		  	//LOG(error);
		  	return paperSizes::A4;
			break;

	}
  }
    		
  bool ExportPdfDialog::getShowScale()  {
  	return groupScale->isChecked();
  	}
  bool ExportPdfDialog::getShowScaleMsg() {
  	return cbScaleUsg->isChecked();
  	}
  bool ExportPdfDialog::getShowDsnFn() {
  	return cbDsnFn->isChecked();
  	}
  bool ExportPdfDialog::getShowGrid()  {
  	return groupGrid->isChecked();
  	}

// Setters
 void  ExportPdfDialog::setShowScale(bool state) {
  	        groupScale->setChecked(state);
  }
 void  ExportPdfDialog::setShowScaleMsg(bool state) {
  	        cbScaleUsg->setChecked(state);
  }
 void  ExportPdfDialog::setShowDsnFn(bool state) {
  	        cbDsnFn->setChecked(state);
  }  
 void ExportPdfDialog::setShowGrid(bool state) {
  	        groupGrid->setChecked(state);
  }

 

  void ExportPdfDialog::setPaperSize(paperSizes paper){
      //setButtonInGroup(sizeButtonGroup, paper2buttons[paper]);
      switch (paper) {
  	case paperSizes::A4:
		rbS_A4->setChecked(TRUE);
		break;
  	case paperSizes::A3:
		rbS_A3->setChecked(TRUE);
		break;
  	case paperSizes::LETTER:
		rbS_Ltr->setChecked(TRUE);
		break;
  	case paperSizes::LEGAL:
		rbS_Leg->setChecked(TRUE);
		break;
  	case paperSizes::TABLOID:
		rbS_Tab->setChecked(TRUE);
		break;
	default:   // provide a sane default.
		rbS_A4->setChecked(TRUE);
		//  LOG(message_group::Export_Warning, Location::NONE, "", "Export Paper Size Unknon( %1$8c )", paper);
  	}	
  }

  void ExportPdfDialog::setOrientation(paperOrientations orient){
      //setButtonInGroup(orientButtonGroup, orientations2buttons[orient]);
      switch (orient) {
  	case paperOrientations::PORTRAIT:
		rbOPort->setChecked(TRUE);
		break;
  	case paperOrientations::LANDSCAPE:
		rbOLand->setChecked(TRUE);
		break;
  	case paperOrientations::AUTO:
		rbOAuto->setChecked(TRUE);
		break;
	default:   // provide a sane default.
		//  LOG(message_group::Export_Warning, Location::NONE, "", "Export Paper Size Unknon( %1$8c )", paper);
		rbOAuto->setChecked(TRUE);
		break;
  	}	
      
  }
  void ExportPdfDialog::setGridSize(double value){
     // setButtonInGroup(gridButtonGroup, grid2buttons[double2gridEnum[value]]);
        //  need to bin to match enums, but tolerate numerical error.
        //  5 values are coded:  2, 2.5, 4, 5, 10; All are integer divisors of 20.
        if (value<2.24) {  // match 2
        	rbGs_2mm->setChecked(TRUE);
        } else if (value<3.1) { // match 2.5
        	rbGs_2r5mm->setChecked(TRUE);     
        } else if (value<4.4) { // match 4 
		rbGs_4mm->setChecked(TRUE);
	} else if (value<7.5) { // match 5
		rbGs_5mm->setChecked(TRUE);
	} else { // match 10
		rbGs_10mm->setChecked(TRUE);
	};
  }
    /*

    enum class ExportPdfDialog::gridSizes{     /*
    2, 2.5, 4, 4.5, 10
  }

  
*/  
   /* 
  std::map<paperSizes, int> ExportPdfDialog::paper2buttons{
   {paperSizes::A4, -2},
   {paperSizes::A3, -3},  
   {paperSizes::LETTER, -4},
   {paperSizes::LEGAL, -5},
   {paperSizes::TABLOID, -6}
  }

  std::map<int, paperSizes> ExportPdfDialog::buttons2paper{
   {-2, paperSizes::A4},
   {-3, paperSizes::A3},  
   {-4, paperSizes::LETTER},
   {-5, paperSizes::LEGAL},
   {-6 paperSizes::TABLOID}
  }

  std::map<paperOrientations, int> ExportPdfDialog::orientations2buttons
  {
   {paperOrientations::PORTRAIT, -2},
   {paperOrientations::LANDSCAPE, -3},  
   {paperOrientations::AUTO, -4}
  }

  std::map<int, paperOrientations> ExportPdfDialog::buttons2orientations{
   {-2, ppaperOrientations::PORTRAIT},
   {-3, paperOrientations::LANDSCAPE},  
   {-4, paperOrientations::AUTO}
  }
 
  std::map<gridSizes, int> ExportPdfDialog::grid2buttons{
   {gridSizes::2, -2},
   {gridSizes::2.5, -3},  
   {gridSizes::4, -4},
   {gridSizes::5, -5},
   {gridSizes::10, -6}
  }


  std::map<int, gridSizes> ExportPdfDialog::buttons2grid{
   {-2, gridSizes::2},
   {-3, gridSizes::2.5},  
   {-4, gridSizes::4},
   {-5, gridSizes::5},
   {-5, gridSizes::10}
  }
  
  std::map<gridSizes, double> ExportPdfDialog::gridEnum2double{
   {gridSizes::2, 2.},
   {gridSizes::2.5, 2.5},  
   {gridSizes::4, 4.},
   {gridSizes::5, 5.},
   {gridSizes::10, 10.}
  }

// a bit dangerous, may need to double and round to int...
  std::map<double, gridSizes> ExportPdfDialog::double2gridEnum{  
   {2., gridSizes::2},
   {2.5, gridSizes::2.5},  
   {4., gridSizes::4},
   {5., gridSizes::5},
   {10., gridSizes::10}
  }
  
     void ExportPdfDialog::setButtonInGroup(QButtonGroup* theGroup, int _id){
  	for (auto button: theGroup->buttons())
        {
            if (id(button) == _id)
            {
                button->setChecked(true);
                break;
            }
        }
  }
*/

void ExportPdfDialog::on_okButton_clicked()
{
  accept();
}

void ExportPdfDialog::on_cancelButton_clicked()
{
  reject();
}

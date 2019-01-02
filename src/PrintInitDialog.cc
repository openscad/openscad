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

#include "PrintInitDialog.h"

PrintInitDialog::PrintInitDialog()
{
	setupUi(this);
	this->textBrowser->setOpenExternalLinks(true);
	this->radioButtonCancel->setChecked(true);
	result = { print_service_t::NONE, false };
}

PrintInitDialog::~PrintInitDialog()
{
}

const PrintServiceResult PrintInitDialog::get_result() const
{
	return result;
}

void PrintInitDialog::on_okButton_clicked()
{
	const bool remember = this->checkBoxRemember->isChecked();
	if (this->radioButtonPrintAThing->isChecked()) {
		result = { print_service_t::PRINT_A_THING, remember };
	} else if (this->radioButtonOctoPrint->isChecked()) {
		result = { print_service_t::OCTOPRINT, remember };
	} else {
		result = { print_service_t::NONE, remember };
	}
	accept();
}

void PrintInitDialog::on_cancelButton_clicked()
{
	reject();
}

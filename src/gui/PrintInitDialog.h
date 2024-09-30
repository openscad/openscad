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

#pragma once

#include "gui/qtgettext.h"
#include "ui_PrintInitDialog.h"

enum class print_service_t { NONE, PRINT_SERVICE, OCTOPRINT, LOCALSLICER };

class PrintInitDialog : public QDialog, public Ui::PrintInitDialog
{
  Q_OBJECT;
public:
  PrintInitDialog();
  static print_service_t getResult();
  static QString serviceName(print_service_t service);

public slots:
  void on_printServiceButton_clicked();
  void on_octoPrintButton_clicked();
  void on_LocalSlicerButton_clicked();
  void on_okButton_clicked();
  void on_cancelButton_clicked();
private:
  print_service_t result;
  QString htmlTemplate;
};

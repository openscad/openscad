/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2025 Clifford Wolf <clifford@clifford.at> and
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

#include "gui/qtgettext.h"       // IWYU pragma: keep
#include "ui_PrintInitDialog.h"  // generated, but needs qtgettext.h

#include <vector>

#include <QList>
#include <QDialog>
#include <QPushButton>

#include "io/export.h"
#include "gui/InitConfigurator.h"

// Property name for remote print servive buttons
constexpr inline auto PROPERTY_NAME = "printServiceName";

enum class print_service_t : std::uint8_t { NONE, PRINT_SERVICE, OCTOPRINT, LOCAL_APPLICATION };

class PrintInitDialog : public QDialog, public Ui::PrintInitDialog, public InitConfigurator
{
  constexpr static auto urlDialog = "qrc:/html/PrintInitDialog.html";
  constexpr static auto urlOctoPrint = "qrc:/html/OctoPrintInfo.html";
  constexpr static auto urlLocalApp = "qrc:/html/LocalApplicationInfo.html";

  Q_OBJECT;

public:
  PrintInitDialog();
  int exec() override;

  print_service_t getServiceType() const;
  QString getServiceName() const;
  FileFormat getFileFormat() const;

public slots:
  void on_pushButtonOctoPrint_clicked();
  void on_pushButtonLocalApplication_clicked();
  void on_comboBoxFileFormat_currentIndexChanged(int);
  void on_checkBoxEnableRemotePrintServices_toggled(bool);
  void on_pushButtonOk_clicked();
  void on_pushButtonCancel_clicked();

private:
  void resetSelection();
  void addRemotePrintServiceButtons();
  void populateFileFormatComboBox(const std::vector<FileFormat>& fileFormats, FileFormat currentFormat);
  void setFileFormat(const std::string& identifier);

  QString htmlTemplate;
  print_service_t selectedPrintService = print_service_t::NONE;
  QString selectedServiceName = "";
  FileFormat selectedFileFormat = FileFormat::ASCII_STL;
  QList<QPushButton *> remoteServiceButtons;
};

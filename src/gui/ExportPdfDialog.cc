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

#include "gui/ExportPdfDialog.h"

#include <QString>
#include <QDialog>

#include "export.h"
#include "Settings.h"
#include "gui/SettingsWriter.h"

using S = Settings::SettingsExportPdf;

ExportPdfDialog::ExportPdfDialog()
{
  setupUi(this);
  this->checkBoxAlwaysShowDialog->setChecked(S::exportPdfAlwaysShowDialog.value());

  initButtonGroup(this->buttonGroupPaperSize, S::exportPdfPaperSize);
  initButtonGroup(this->buttonGroupOrientation, S::exportPdfOrientation);

  // Get current settings or defaults modify the two enums (next two rows) to explicitly use default by lookup to string (see the later set methods).
  this->checkBoxShowFilename->setChecked(S::exportPdfShowFilename.value());
  this->groupScale->setChecked(S::exportPdfShowScale.value());
  this->checkBoxShowScaleMessage->setChecked(S::exportPdfShowScaleMessage.value());
  this->groupGrid->setChecked(S::exportPdfShowGrid.value());
  this->setGridSize(S::exportPdfGridSize.value());

	groupMetaData->setChecked(S::exportPdfAddMetaData.value());
	initMetaData(nullptr, this->lineEditMetaDataTitle, nullptr, S::exportPdfMetaDataTitle);
	initMetaData(this->checkBoxMetaDataAuthor, this->lineEditMetaDataAuthor, &S::exportPdfAddMetaDataAuthor, S::exportPdfMetaDataAuthor);
	initMetaData(this->checkBoxMetaDataSubject, this->lineEditMetaDataSubject, &S::exportPdfAddMetaDataSubject, S::exportPdfMetaDataSubject);
	initMetaData(this->checkBoxMetaDataKeywords, this->lineEditMetaDataKeywords, &S::exportPdfAddMetaDataKeywords, S::exportPdfMetaDataKeywords);
}

int ExportPdfDialog::exec()
{
  bool showDialog = this->checkBoxAlwaysShowDialog->isChecked();
  if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
  	showDialog = true;
  }

  const auto result = showDialog ? QDialog::exec() : QDialog::Accepted;

  if (result == QDialog::Accepted) {
  	S::exportPdfAlwaysShowDialog.setValue(this->checkBoxAlwaysShowDialog->isChecked());
    applyButtonGroup(this->buttonGroupPaperSize, S::exportPdfPaperSize);
    applyButtonGroup(this->buttonGroupOrientation, S::exportPdfOrientation);
    S::exportPdfShowFilename.setValue(this->checkBoxShowFilename->isChecked());
    S::exportPdfShowScale.setValue(this->groupScale->isChecked());
    S::exportPdfShowScaleMessage.setValue(this->checkBoxShowScaleMessage->isChecked());
    S::exportPdfShowGrid.setValue(this->groupGrid->isChecked());
    S::exportPdfGridSize.setValue(getGridSize());
    S::exportPdfAddMetaData.setValue(this->groupMetaData->isChecked());
    applyMetaData(nullptr, this->lineEditMetaDataTitle, nullptr, S::exportPdfMetaDataTitle);
    applyMetaData(this->checkBoxMetaDataAuthor, this->lineEditMetaDataAuthor, &S::exportPdfAddMetaDataAuthor, S::exportPdfMetaDataAuthor);
    applyMetaData(this->checkBoxMetaDataSubject, this->lineEditMetaDataSubject, &S::exportPdfAddMetaDataSubject, S::exportPdfMetaDataSubject);
    applyMetaData(this->checkBoxMetaDataKeywords, this->lineEditMetaDataKeywords, &S::exportPdfAddMetaDataKeywords, S::exportPdfMetaDataKeywords);
    Settings::Settings::visit(SettingsWriter());
  }

  return result;
}

double ExportPdfDialog::getGridSize() const
{
  const auto button = buttonGroupGridSize->checkedButton();
  return button ? button->property(Settings::PROPERTY_SELECTED_VALUE).toDouble() : 10.0;
}

void ExportPdfDialog::setGridSize(double value)
{
  for (auto button : buttonGroupGridSize->buttons()) {
    const auto buttonValue = button->property(Settings::PROPERTY_SELECTED_VALUE).toDouble();
    if (std::abs(buttonValue - value) < 0.5) {
      button->setChecked(true);
      return;
    }
  }
  rbGs_10mm->setChecked(true); // default
}

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
#include <QColorDialog>

#include "io/export.h"
#include "core/Settings.h"
#include "gui/UIUtils.h"
#include "gui/SettingsWriter.h"

using S = Settings::SettingsExportPdf;

ExportPdfDialog::ExportPdfDialog()
{
  setupUi(this);
  this->checkBoxAlwaysShowDialog->setChecked(S::exportPdfAlwaysShowDialog.value());

  initButtonGroup(this->buttonGroupPaperSize, S::exportPdfPaperSize);
  initButtonGroup(this->buttonGroupOrientation, S::exportPdfOrientation);

  // Get current settings or defaults
  this->checkBoxShowFilename->setChecked(S::exportPdfShowFilename.value());
  this->groupScale->setChecked(S::exportPdfShowScale.value());
  this->checkBoxShowScaleMessage->setChecked(S::exportPdfShowScaleMessage.value());
  this->groupGrid->setChecked(S::exportPdfShowGrid.value());

  // Initialize grid size from settings
  const auto gridSize = S::exportPdfGridSize.value();
  for (auto *button : buttonGroupGridSize->buttons()) {
    if (button->property("_selected_value").toDouble() == gridSize) {
      button->setChecked(true);
      break;
    }
  }

  // Fill settings
  this->checkBoxEnableFill->setChecked(S::exportPdfFill.value());
  this->fillColor = QColor(QString::fromStdString(S::exportPdfFillColor.value()));
  updateFillColor(this->fillColor);
  updateFillControlsEnabled();

  // Stroke settings
  this->checkBoxEnableStroke->setChecked(S::exportPdfStroke.value());
  this->strokeColor = QColor(QString::fromStdString(S::exportPdfStrokeColor.value()));
  this->doubleSpinBoxStrokeWidth->setValue(S::exportPdfStrokeWidth.value());
  updateStrokeColor(this->strokeColor);
  updateStrokeControlsEnabled();

  groupMetaData->setChecked(S::exportPdfAddMetaData.value());
  initMetaData(nullptr, this->lineEditMetaDataTitle, nullptr, S::exportPdfMetaDataTitle);
  initMetaData(this->checkBoxMetaDataAuthor, this->lineEditMetaDataAuthor,
               &S::exportPdfAddMetaDataAuthor, S::exportPdfMetaDataAuthor);
  initMetaData(this->checkBoxMetaDataSubject, this->lineEditMetaDataSubject,
               &S::exportPdfAddMetaDataSubject, S::exportPdfMetaDataSubject);
  initMetaData(this->checkBoxMetaDataKeywords, this->lineEditMetaDataKeywords,
               &S::exportPdfAddMetaDataKeywords, S::exportPdfMetaDataKeywords);
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
    S::exportPdfFill.setValue(this->checkBoxEnableFill->isChecked());
    S::exportPdfFillColor.setValue(this->fillColor.name().toStdString());
    S::exportPdfStroke.setValue(this->checkBoxEnableStroke->isChecked());
    S::exportPdfStrokeColor.setValue(this->strokeColor.name().toStdString());
    S::exportPdfStrokeWidth.setValue(this->doubleSpinBoxStrokeWidth->value());
    S::exportPdfAddMetaData.setValue(this->groupMetaData->isChecked());
    applyMetaData(nullptr, this->lineEditMetaDataTitle, nullptr, S::exportPdfMetaDataTitle);
    applyMetaData(this->checkBoxMetaDataAuthor, this->lineEditMetaDataAuthor,
                  &S::exportPdfAddMetaDataAuthor, S::exportPdfMetaDataAuthor);
    applyMetaData(this->checkBoxMetaDataSubject, this->lineEditMetaDataSubject,
                  &S::exportPdfAddMetaDataSubject, S::exportPdfMetaDataSubject);
    applyMetaData(this->checkBoxMetaDataKeywords, this->lineEditMetaDataKeywords,
                  &S::exportPdfAddMetaDataKeywords, S::exportPdfMetaDataKeywords);
    Settings::Settings::visit(SettingsWriter());
  }

  return result;
}

double ExportPdfDialog::getGridSize() const
{
  const auto button = buttonGroupGridSize->checkedButton();
  return button ? button->property("_selected_value").toDouble() : 10.0;
}

void ExportPdfDialog::updateFillColor(const QColor& color)
{
  this->fillColor = color;
  QString styleSheet = QString("QLabel { background-color: %1; }").arg(color.name());
  this->labelFillColor->setStyleSheet(styleSheet);
}

void ExportPdfDialog::updateStrokeColor(const QColor& color)
{
  this->strokeColor = color;
  QString styleSheet = QString("QLabel { background-color: %1; }").arg(color.name());
  this->labelStrokeColor->setStyleSheet(styleSheet);
}

void ExportPdfDialog::updateFillControlsEnabled()
{
  bool enabled = this->checkBoxEnableFill->isChecked();
  this->labelFillColor->setEnabled(enabled);
  this->toolButtonFillColor->setEnabled(enabled);
  this->toolButtonFillColorReset->setEnabled(enabled);
}

void ExportPdfDialog::updateStrokeControlsEnabled()
{
  bool enabled = this->checkBoxEnableStroke->isChecked();
  this->labelStrokeColor->setEnabled(enabled);
  this->toolButtonStrokeColor->setEnabled(enabled);
  this->toolButtonStrokeColorReset->setEnabled(enabled);
  this->labelStrokeWidth->setEnabled(enabled);
  this->doubleSpinBoxStrokeWidth->setEnabled(enabled);
  this->toolButtonStrokeWidthReset->setEnabled(enabled);
}

void ExportPdfDialog::on_toolButtonFillColor_clicked()
{
  QColor color = QColorDialog::getColor(this->fillColor, this);
  if (color.isValid()) {
    updateFillColor(color);
  }
}

void ExportPdfDialog::on_toolButtonStrokeColor_clicked()
{
  QColor color = QColorDialog::getColor(this->strokeColor, this);
  if (color.isValid()) {
    updateStrokeColor(color);
  }
}

void ExportPdfDialog::on_toolButtonFillColorReset_clicked()
{
  updateFillColor(QColor(QString::fromStdString(S::exportPdfFillColor.defaultValue())));
}

void ExportPdfDialog::on_toolButtonStrokeColorReset_clicked()
{
  updateStrokeColor(QColor(QString::fromStdString(S::exportPdfStrokeColor.defaultValue())));
}

void ExportPdfDialog::on_toolButtonStrokeWidthReset_clicked()
{
  this->doubleSpinBoxStrokeWidth->setValue(this->defaultStrokeWidth);
}

void ExportPdfDialog::on_checkBoxEnableFill_toggled(bool checked) { updateFillControlsEnabled(); }

void ExportPdfDialog::on_checkBoxEnableStroke_toggled(bool checked) { updateStrokeControlsEnabled(); }

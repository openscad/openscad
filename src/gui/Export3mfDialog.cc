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

#include "gui/Export3mfDialog.h"

#include <string>
#include <QString>
#include <QCheckBox>
#include <QColor>
#include <QDialog>
#include <QColorDialog>
#include <QLineEdit>

#include "io/export.h"
#include "io/lib3mf_utils.h"
#include "core/Settings.h"
#include "gui/UIUtils.h"
#include "gui/SettingsWriter.h"

using S = Settings::SettingsExport3mf;
using SEBool = Settings::SettingsEntryBool;
using SEString = Settings::SettingsEntryString;

Export3mfDialog::Export3mfDialog()
{
  setupUi(this);
  this->checkBoxAlwaysShowDialog->setChecked(S::export3mfAlwaysShowDialog.value());
  initButtonGroup(this->buttonGroupColors, S::export3mfColorMode);
  initButtonGroup(this->buttonGroupUnit, S::export3mfUnit);
  this->color = QColor(QString::fromStdString(S::export3mfColor.value()));
  this->labelColorsSelected->setStyleSheet(UIUtils::getBackgroundColorStyleSheet(this->color));
  this->spinBoxDecimalPrecision->setValue(S::export3mfDecimalPrecision.value());
  initComboBox(this->comboBoxMaterialType, S::export3mfMaterialType);

  groupMetaData->setChecked(S::export3mfAddMetaData.value());
  initMetaData(nullptr, this->lineEditMetaDataTitle, nullptr, S::export3mfMetaDataTitle);
  initMetaData(this->checkBoxMetaDataDesigner, this->lineEditMetaDataDesigner,
               &S::export3mfAddMetaDataDesigner, S::export3mfMetaDataDesigner);
  initMetaData(this->checkBoxMetaDataDescription, this->lineEditMetaDataDescription,
               &S::export3mfAddMetaDataDescription, S::export3mfMetaDataDescription);
  initMetaData(this->checkBoxMetaDataCopyright, this->lineEditMetaDataCopyright,
               &S::export3mfAddMetaDataCopyright, S::export3mfMetaDataCopyright);
  initMetaData(this->checkBoxMetaDataLicenseTerms, this->lineEditMetaDataLicenseTerms,
               &S::export3mfAddMetaDataLicenseTerms, S::export3mfMetaDataLicenseTerms);
  initMetaData(this->checkBoxMetaDataRating, this->lineEditMetaDataRating,
               &S::export3mfAddMetaDataRating, S::export3mfMetaDataRating);

  const auto library_version = get_lib3mf_version();
  if (library_version.compare(0, 2, "1.") == 0) {
    this->spinBoxDecimalPrecision->setEnabled(false);
    this->toolButtonDecimalPrecisionReset->setEnabled(false);
    this->labelDecimalPrecision->setEnabled(false);
    this->spinBoxDecimalPrecision->setToolTip(
      _("This OpenSCAD build uses lib3mf version 1. Setting the decimal precision for export needs "
        "version 2 or later."));
    this->toolButtonDecimalPrecisionReset->setToolTip("");
  }
}

void Export3mfDialog::updateColor(const QColor& color)
{
  this->color = color;
  this->labelColorsSelected->setStyleSheet(UIUtils::getBackgroundColorStyleSheet(this->color));
}

int Export3mfDialog::exec()
{
  bool showDialog = this->checkBoxAlwaysShowDialog->isChecked();
  if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
    showDialog = true;
  }

  const auto result = showDialog ? QDialog::exec() : QDialog::Accepted;

  if (result == QDialog::Accepted) {
    S::export3mfAlwaysShowDialog.setValue(this->checkBoxAlwaysShowDialog->isChecked());
    applyButtonGroup(this->buttonGroupColors, S::export3mfColorMode);
    applyButtonGroup(this->buttonGroupUnit, S::export3mfUnit);
    S::export3mfColor.setValue(this->color.toRgb().name().toStdString());
    S::export3mfMaterialType.setIndex(this->comboBoxMaterialType->currentIndex());
    S::export3mfDecimalPrecision.setValue(this->spinBoxDecimalPrecision->value());
    S::export3mfAddMetaData.setValue(this->groupMetaData->isChecked());
    applyMetaData(nullptr, this->lineEditMetaDataTitle, nullptr, S::export3mfMetaDataTitle);
    applyMetaData(this->checkBoxMetaDataDesigner, this->lineEditMetaDataDesigner,
                  &S::export3mfAddMetaDataDesigner, S::export3mfMetaDataDesigner);
    applyMetaData(this->checkBoxMetaDataDescription, this->lineEditMetaDataDescription,
                  &S::export3mfAddMetaDataDescription, S::export3mfMetaDataDescription);
    applyMetaData(this->checkBoxMetaDataCopyright, this->lineEditMetaDataCopyright,
                  &S::export3mfAddMetaDataCopyright, S::export3mfMetaDataCopyright);
    applyMetaData(this->checkBoxMetaDataLicenseTerms, this->lineEditMetaDataLicenseTerms,
                  &S::export3mfAddMetaDataLicenseTerms, S::export3mfMetaDataLicenseTerms);
    applyMetaData(this->checkBoxMetaDataRating, this->lineEditMetaDataRating,
                  &S::export3mfAddMetaDataRating, S::export3mfMetaDataRating);
    Settings::Settings::visit(SettingsWriter());
  }

  return result;
}

void Export3mfDialog::on_toolButtonColorsSelected_clicked()
{
  updateColor(QColorDialog::getColor(this->color));
}

void Export3mfDialog::on_toolButtonColorsSelectedReset_clicked()
{
  updateColor(QColor(QString::fromStdString(S::export3mfColor.defaultValue())));
}

void Export3mfDialog::on_toolButtonDecimalPrecisionReset_clicked()
{
  this->spinBoxDecimalPrecision->setValue(S::export3mfDecimalPrecision.defaultValue());
}

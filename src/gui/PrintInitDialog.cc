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

#include "gui/PrintInitDialog.h"

#include <vector>
#include <string>

#include <QDialog>
#include <QString>
#include <QPushButton>

#include "io/export.h"
#include "core/Settings.h"
#include "gui/PrintService.h"

using S = Settings::Settings;

namespace {

QString toString(print_service_t printServiceType)
{
  switch (printServiceType) {
  case print_service_t::PRINT_SERVICE:     return "PRINT_SERVICE";
  case print_service_t::OCTOPRINT:         return "OCTOPRINT";
  case print_service_t::LOCAL_APPLICATION: return "LOCAL_APPLICATION";
  default:                                 return "NONE";
  }
}

print_service_t fromString(const std::string& printServiceType)
{
  if (printServiceType == "PRINT_SERVICE") {
    return print_service_t::PRINT_SERVICE;
  } else if (printServiceType == "OCTOPRINT") {
    return print_service_t::OCTOPRINT;
  } else if (printServiceType == "LOCAL_APPLICATION") {
    return print_service_t::LOCAL_APPLICATION;
  } else return print_service_t::NONE;
}

}  // namespace

void PrintInitDialog::populateFileFormatComboBox(const std::vector<FileFormat>& fileFormats,
                                                 FileFormat currentFormat)
{
  this->comboBoxFileFormat->clear();
  for (const auto& fileFormat : fileFormats) {
    const FileFormatInfo& info = fileformat::info(fileFormat);
    this->comboBoxFileFormat->addItem(QString::fromStdString(info.description),
                                      QString::fromStdString(info.identifier));
    if (fileFormat == currentFormat) {
      this->comboBoxFileFormat->setCurrentIndex(this->comboBoxFileFormat->count() - 1);
    }
  }
}

PrintInitDialog::PrintInitDialog()
{
  setupUi(this);

  resetSelection();

  this->checkBoxAlwaysShowDialog->setChecked(S::printServiceAlwaysShowDialog.value());

  // triggers addRemotePrintServiceButtons() if config value is "true"
  const auto remoteServicesEnabled = S::enableRemotePrintServices.value();
  this->checkBoxEnableRemotePrintServices->setChecked(remoteServicesEnabled);

  if (remoteServicesEnabled && PrintService::getPrintServices().empty()) {
    LOG(message_group::UI_Warning, "No external print services found");
  }

  const auto& service = S::defaultPrintService.value();
  const print_service_t printService = fromString(service);
  if (printService != print_service_t::NONE) {
    this->selectedPrintService = printService;
    const auto& printServiceName = S::printServiceName.value();
    this->selectedServiceName = QString::fromStdString(printServiceName);

    switch (printService) {
    case print_service_t::PRINT_SERVICE: {
      for (const auto& button : this->buttonGroup->buttons()) {
        const auto& name = button->property(PROPERTY_NAME);
        if (this->selectedServiceName == name.toString()) {
          button->click();
          break;
        }
      }
    } break;
    case print_service_t::OCTOPRINT:         on_pushButtonOctoPrint_clicked(); break;
    case print_service_t::LOCAL_APPLICATION: on_pushButtonLocalApplication_clicked(); break;
    default:                                 break;
    }
  }
}

void PrintInitDialog::resetSelection()
{
  this->textBrowser->setSource(QUrl{urlDialog});
  this->pushButtonOk->setEnabled(false);
  this->comboBoxFileFormat->setEnabled(false);
  this->comboBoxFileFormat->setCurrentIndex(-1);
  this->buttonGroup->setExclusive(false);
  for (auto button : this->buttonGroup->buttons()) {
    button->setChecked(false);
  }
  this->buttonGroup->setExclusive(true);
}

void PrintInitDialog::addRemotePrintServiceButtons()
{
  for (const auto& printServiceItem : PrintService::getPrintServices()) {
    const auto& key = printServiceItem.first;
    const auto& printService = printServiceItem.second;
    auto button = new QPushButton(printService->getDisplayName(), this);
    remoteServiceButtons.push_back(button);
    button->setCheckable(true);
    button->setAutoDefault(false);
    button->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    button->setProperty(PROPERTY_NAME, QVariant(QString::fromStdString(key)));
    buttonGroup->addButton(button);
    this->printServiceLayout->insertWidget(this->printServiceLayout->count(), button);
    connect(button, &QPushButton::clicked, this, [&]() {
      FileFormat currentFormat = FileFormat::ASCII_STL;
      fileformat::fromIdentifier(S::printServiceFileFormat.value(), currentFormat);
      this->textBrowser->setHtml(printService->getInfoHtml());
      this->populateFileFormatComboBox(printService->getFileFormats(), currentFormat);
      this->selectedPrintService = print_service_t::PRINT_SERVICE;
      this->selectedServiceName = QString::fromStdString(key);
      this->comboBoxFileFormat->setEnabled(true);
      this->pushButtonOk->setEnabled(true);
    });
  }
}

void PrintInitDialog::on_checkBoxEnableRemotePrintServices_toggled(bool checked)
{
  S::enableRemotePrintServices.setValue(checked);
  writeSettings();

  if (checked) {
    if (remoteServiceButtons.empty()) {
      addRemotePrintServiceButtons();
    }
  } else {
    for (const auto button : remoteServiceButtons) {
      this->buttonGroup->removeButton(button);
      this->printServiceLayout->removeWidget(button);
      button->deleteLater();
    }
    remoteServiceButtons.clear();
  }

  resetSelection();
}

void PrintInitDialog::on_pushButtonOctoPrint_clicked()
{
  this->textBrowser->setSource(QUrl{urlOctoPrint});
  initComboBox(this->comboBoxFileFormat, S::octoPrintFileFormat);
  this->on_comboBoxFileFormat_currentIndexChanged(this->comboBoxFileFormat->currentIndex());

  this->selectedPrintService = print_service_t::OCTOPRINT;
  this->selectedServiceName = "";

  this->comboBoxFileFormat->setEnabled(true);
  this->pushButtonOk->setEnabled(true);
  const auto fileFormatIdentifier = fileformat::info(this->getFileFormat()).identifier;
  PRINTDB("OCTOPRINT: %s", fileFormatIdentifier);
}

void PrintInitDialog::on_pushButtonLocalApplication_clicked()
{
  // TODO: Instead of forcing people to use Preferences, we should add UI here
  // to select external program.
  this->textBrowser->setSource(QUrl{urlLocalApp});

  initComboBox(this->comboBoxFileFormat, S::localAppFileFormat);
  this->on_comboBoxFileFormat_currentIndexChanged(this->comboBoxFileFormat->currentIndex());
  this->selectedPrintService = print_service_t::LOCAL_APPLICATION;
  this->selectedServiceName = "";

  this->comboBoxFileFormat->setEnabled(true);
  this->pushButtonOk->setEnabled(true);
  const auto fileFormatIdentifier = fileformat::info(this->getFileFormat()).identifier;
  PRINTDB("LOCAL_APPLICATION: %s", fileFormatIdentifier);
}

void PrintInitDialog::setFileFormat(const std::string& id)
{
  FileFormat fileFormat = FileFormat::ASCII_STL;
  if (!fileformat::fromIdentifier(id, fileFormat)) {
    // FIXME: When would this error happen? Do we need to handle it?
    LOG("fileformat::fromIdentifier error: id '%1$s' not recognized", id);
  }
  this->selectedFileFormat = fileFormat;
}

void PrintInitDialog::on_comboBoxFileFormat_currentIndexChanged(int index)
{
  if (index >= 0) {
    const auto id = this->comboBoxFileFormat->currentData().toString().toStdString();
    this->setFileFormat(id);
  }
}

void PrintInitDialog::on_pushButtonOk_clicked()
{
  const QString defaultPrintServiceString = toString(this->selectedPrintService);
  S::defaultPrintService.setValue(defaultPrintServiceString.toStdString());
  S::printServiceName.setValue(this->selectedServiceName.toStdString());

  const auto fileFormatIdentifier = fileformat::info(this->getFileFormat()).identifier;
  switch (this->selectedPrintService) {
  case print_service_t::PRINT_SERVICE:     S::printServiceFileFormat.setValue(fileFormatIdentifier); break;
  case print_service_t::OCTOPRINT:         S::octoPrintFileFormat.setValue(fileFormatIdentifier); break;
  case print_service_t::LOCAL_APPLICATION: S::localAppFileFormat.setValue(fileFormatIdentifier); break;
  default:                                 break;
  }
  // FIXME: Add support for executable selection
  writeSettings();
  accept();
}

void PrintInitDialog::on_pushButtonCancel_clicked() { reject(); }

int PrintInitDialog::exec()
{
  bool showDialog = this->checkBoxAlwaysShowDialog->isChecked();

  // Show the dialog if icon was shift-clicked, if no print service is selected,
  // or if the selected print service is not available.
  const bool isShiftKeyModifier = (QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0;
  const bool isNoPrintService = this->selectedPrintService == print_service_t::NONE;
  const bool isRemotePrintService = this->selectedPrintService == print_service_t::PRINT_SERVICE;
  const auto printService = PrintService::getPrintService(this->selectedServiceName.toStdString());
  const bool noRemotePrintServiceSelected = isRemotePrintService && !printService;
  if (isShiftKeyModifier || isNoPrintService || noRemotePrintServiceSelected) {
    showDialog = true;
  }

  const auto result = showDialog ? QDialog::exec() : QDialog::Accepted;

  if (showDialog && result == QDialog::Accepted) {
    S::printServiceAlwaysShowDialog.setValue(this->checkBoxAlwaysShowDialog->isChecked());
    writeSettings();
  }

  return result;
}

print_service_t PrintInitDialog::getServiceType() const { return this->selectedPrintService; }

QString PrintInitDialog::getServiceName() const { return this->selectedServiceName; }

FileFormat PrintInitDialog::getFileFormat() const { return this->selectedFileFormat; }

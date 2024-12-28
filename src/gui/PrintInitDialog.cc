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

#include "gui/PrintInitDialog.h"

#include <QDialog>
#include <QString>

#include "export.h"
#include "gui/PrintService.h"
#include "gui/QSettingsCached.h"
#include "gui/Settings.h"

namespace {

QString toString(print_service_t printServiceType) {
  switch (printServiceType) {
  case print_service_t::PRINT_SERVICE:
    return "PRINT_SERVICE";
  case print_service_t::OCTOPRINT:
    return "OCTOPRINT";
  case print_service_t::LOCALSLICER:
    return "LOCALSLICER";
  default:
    return "NONE";
  }
}

print_service_t fromString(const QString &printServiceType) {
  if (printServiceType == "PRINT_SERVICE") {
    return print_service_t::PRINT_SERVICE;
  } else if (printServiceType == "OCTOPRINT") {
    return print_service_t::OCTOPRINT;
  } else if (printServiceType == "LOCALSLICER") {
    return print_service_t::LOCALSLICER;
  } else
    return print_service_t::NONE;
}

} // namespace

void PrintInitDialog::populateFileFormatComboBox(
    const std::vector<FileFormat> &fileFormats, FileFormat currentFormat) {
  this->fileFormatComboBox->clear();
  for (const auto &fileFormat : fileFormats) {
    const FileFormatInfo &info = fileformat::info(fileFormat);
    this->fileFormatComboBox->addItem(QString::fromStdString(info.description),
                                      QString::fromStdString(info.identifier));
    if (fileFormat == currentFormat) {
      this->fileFormatComboBox->setCurrentIndex(
          this->fileFormatComboBox->count() - 1);
    }
  }
}

PrintInitDialog::PrintInitDialog() {
  setupUi(this);

  this->textBrowser->setSource(QUrl{"qrc:/html/PrintInitDialog.html"});
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  this->fileFormatComboBox->setEnabled(false);

  for (const auto &printServiceItem : PrintService::getPrintServices()) {
    const auto &key = printServiceItem.first;
    const auto &printService = printServiceItem.second;
    auto button = new QPushButton(printService->getDisplayName(), this);
    button->setCheckable(true);
    button->setAutoExclusive(true);
    this->printServiceLayout->insertWidget(0, button);
    connect(button, &QPushButton::clicked, this, [&]() {
      const QSettingsCached settings;
      FileFormat currentFormat = FileFormat::ASCII_STL;
      fileformat::fromIdentifier(
          settings.value("printing/printServiceFileFormat")
              .toString()
              .toStdString(),
          currentFormat);

      this->textBrowser->setHtml(printService->getInfoHtml());
      this->populateFileFormatComboBox(printService->getFileFormats(),
                                       currentFormat);

      this->selectedPrintService = print_service_t::PRINT_SERVICE;
      this->selectedServiceName = QString::fromStdString(key);
      this->fileFormatComboBox->setEnabled(true);
      this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    });
  }

  if (PrintService::getPrintServices().empty()) {
    LOG(message_group::UI_Warning, "No external print services found");
  }
}

void PrintInitDialog::on_octoPrintButton_clicked() {
  const QSettingsCached settings;

  this->textBrowser->setSource(QUrl{"qrc:/html/OctoPrintInfo.html"});
  initComboBox(this->fileFormatComboBox,
               Settings::Settings::octoPrintFileFormat);
  this->on_fileFormatComboBox_currentIndexChanged(
      this->fileFormatComboBox->currentIndex());

  this->selectedPrintService = print_service_t::OCTOPRINT;
  this->selectedServiceName = "";

  this->fileFormatComboBox->setEnabled(true);
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  PRINTD("OCTOPRINT");
}

void PrintInitDialog::on_LocalSlicerButton_clicked() {
  const QSettingsCached settings;

  // TODO: Instead of forcing people to use Preferences, we should add UI here
  // to select external program.
  this->textBrowser->setSource(QUrl{"qrc:/html/LocalSlicerInfo.html"});

  initComboBox(this->fileFormatComboBox,
               Settings::Settings::localSlicerFileFormat);
  this->on_fileFormatComboBox_currentIndexChanged(
      this->fileFormatComboBox->currentIndex());

  this->selectedPrintService = print_service_t::LOCALSLICER;
  this->selectedServiceName = "";

  this->fileFormatComboBox->setEnabled(true);
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  PRINTD("LOCALSLICER");
}

void PrintInitDialog::on_fileFormatComboBox_currentIndexChanged(int index) {
  if (index >= 0) {
    FileFormat fileFormat = FileFormat::ASCII_STL;
    std::string identifier =
        this->fileFormatComboBox->currentData().toString().toStdString();
    if (!fileformat::fromIdentifier(identifier, fileFormat)) {
      // FIXME: When would this error happen? Do we need to handle it?
      LOG("fileformat::fromIdentifier error: identifier '%2$s' not recognized "
          "(combobox index %2$d)",
          identifier, index);
    }
    this->selectedFileFormat = fileFormat;
  }
}

void PrintInitDialog::on_buttonBox_accepted() {
  if (this->checkBoxRememberSelection->isChecked()) {
    QSettingsCached settings;

    const QString defaultPrintServiceString =
        toString(this->selectedPrintService);
    settings.setValue(
        QString::fromStdString(Settings::Settings::defaultPrintService.key()),
        defaultPrintServiceString);

    settings.setValue(
        QString::fromStdString(Settings::Settings::printServiceName.key()),
        this->selectedServiceName);

    const QString fileFormatIdentifier = QString::fromStdString(
        fileformat::info(this->getFileFormat()).identifier);
    switch (this->selectedPrintService) {
    case print_service_t::PRINT_SERVICE:
      settings.setValue(QString::fromStdString(
                            Settings::Settings::printServiceFileFormat.key()),
                        fileFormatIdentifier);
      break;
    case print_service_t::OCTOPRINT:
      settings.setValue(
          QString::fromStdString(Settings::Settings::octoPrintFileFormat.key()),
          fileFormatIdentifier);
    case print_service_t::LOCALSLICER:
      settings.setValue(QString::fromStdString(
                            Settings::Settings::localSlicerFileFormat.key()),
                        fileFormatIdentifier);
    default:
      break;
    }
    // FIXME: Add support for executable selection
    // * localSlicerExecutable
  }
  accept();
}

void PrintInitDialog::on_buttonBox_rejected() { reject(); }

int PrintInitDialog::exec() {
  const QSettingsCached settings;
  const QString service = settings.value("printing/printService").toString();
  const print_service_t printService = fromString(service);
  if (printService != print_service_t::NONE) {
    this->selectedPrintService = printService;
    this->selectedServiceName =
        settings.value("printing/printServiceName").toString();

    QString fileFormatString;
    switch (printService) {
    case print_service_t::PRINT_SERVICE:
      fileFormatString =
          settings.value("printing/printServiceFileFormat").toString();
      break;
    case print_service_t::OCTOPRINT:
      fileFormatString =
          settings.value("printing/octoPrintFileFormat").toString();
      break;
    case print_service_t::LOCALSLICER:
      fileFormatString =
          settings.value("printing/localSlicerFileFormat").toString();
      break;
    default:
      break;
    }
    if (!fileformat::fromIdentifier(fileFormatString.toStdString(),
                                    this->selectedFileFormat)) {
      LOG("TODO: Error unsupported file format");
    }

    return QDialog::Accepted;
  }

  return QDialog::exec();
}

print_service_t PrintInitDialog::getServiceType() const {
  return this->selectedPrintService;
}

QString PrintInitDialog::getServiceName() const {
  return this->selectedServiceName;
}

FileFormat PrintInitDialog::getFileFormat() const {
  return this->selectedFileFormat;
}

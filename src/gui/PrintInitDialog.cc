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
#include "gui/Settings.h"
#include "gui/PrintService.h"
#include "gui/QSettingsCached.h"

PrintInitDialog::PrintInitDialog()
{
  setupUi(this);

  this->textBrowser->setSource(QUrl{"qrc:/html/PrintInitDialog.html"});
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  for (const auto &printServiceItem : PrintService::getPrintServices()) {
    const auto& key = printServiceItem.first;
    const auto& printService = printServiceItem.second;
    auto button = new QPushButton(printService->getDisplayName(), this);
    button->setCheckable(true);
    button->setAutoExclusive(true);
    this->printServiceLayout->insertWidget(0, button);
    connect(button, &QPushButton::clicked, this, [&](){
      this->textBrowser->setHtml(printService->getInfoHtml());
      this->fileFormatComboBox->clear();
      for (const auto fileFormat : {FileFormat::BINARY_STL, FileFormat::ASCII_STL}) {
        this->fileFormatComboBox->addItem(
          QString::fromStdString(fileformat::info(fileFormat).description),
          QString::fromStdString(fileformat::info(fileFormat).identifier));
      }
      // TODO: Use format stored in settings as selected value

      this->selectedPrintService = print_service_t::PRINT_SERVICE;
      this->selectedServiceName = QString::fromStdString(key);
      this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    });
  }

  if (PrintService::getPrintServices().empty()) {
    LOG(message_group::UI_Warning, "No external print services found");
  }
}

void PrintInitDialog::on_octoPrintButton_clicked()
{
  this->textBrowser->setSource(QUrl{"qrc:/html/OctoPrintInfo.html"});
  this->fileFormatComboBox->clear();

  for (const auto fileFormat : {FileFormat::BINARY_STL, FileFormat::ASCII_STL}) {
    this->fileFormatComboBox->addItem(
      QString::fromStdString(fileformat::info(fileFormat).description),
      QString::fromStdString(fileformat::info(fileFormat).identifier));
  }
  // TODO: Use format stored in settings as selected value

  this->selectedPrintService = print_service_t::OCTOPRINT;
  this->selectedServiceName = "";

  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  PRINTD("OCTOPRINT");
}

void PrintInitDialog::on_LocalSlicerButton_clicked()
{
  // TODO: Instead of forcing people to use Preferences, we should add UI here to:
  // 2. Select external program
  this->textBrowser->setSource(QUrl{"qrc:/html/LocalSlicerInfo.html"});

  this->fileFormatComboBox->clear();
  for (const auto fileFormat : fileformat::all()) {
    if (fileformat::is3D(fileFormat)) {
      this->fileFormatComboBox->addItem(
        QString::fromStdString(fileformat::info(fileFormat).description),
        QString::fromStdString(fileformat::info(fileFormat).identifier));
    }
  }
  // TODO: Use format stored in settings as selected value
  this->selectedPrintService = print_service_t::LOCALSLICER;
  this->selectedServiceName = "";

  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  PRINTD("LOCALSLICER");
}

void PrintInitDialog::on_buttonBox_accepted()
{
  if (this->checkBoxRememberSelection->isChecked()) {
    QSettingsCached settings;
    // TODO: Store values in settings
  }
  accept();
}

void PrintInitDialog::on_buttonBox_rejected()
{
  reject();
}

int PrintInitDialog::exec()
{
  const QSettingsCached settings;
  auto service = settings.value(QString::fromStdString(Settings::Settings::defaultPrintService.key())).toString();
  if (isValidPrintServiceKey(service.toStdString())) {
    // TODO: Populate file format
    return QDialog::Accepted;
  }

  return QDialog::exec();
}

print_service_t PrintInitDialog::getServiceType() const
{
  return this->selectedPrintService;
}

QString PrintInitDialog::getServiceName() const
{
  return this->selectedServiceName;
}

FileFormat PrintInitDialog::getFileFormat() const
{
  FileFormat fileFormat = FileFormat::ASCII_STL;
  if (!fileformat::fromIdentifier(this->fileFormatComboBox->currentData().toString().toStdString(), fileFormat)) {
    // FIXME: When would this error happen? Do we need to handle it?
    LOG("fileformat::fromIdentifier error");
  }
  return fileFormat;
}

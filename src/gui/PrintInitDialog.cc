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

#include "gui/Settings.h"
#include "gui/PrintService.h"
#include "gui/QSettingsCached.h"

PrintInitDialog::PrintInitDialog()
{
  setupUi(this);

  this->textBrowser->setSource(QUrl{"qrc:/html/PrintInitDialog.html"});
  this->result = "NONE";

  this->okButton->setEnabled(false);

  for (const auto &printServiceItem : PrintService::getPrintServices()) {
    const auto& key = printServiceItem.first;
    const auto& printService = printServiceItem.second;
    auto button = new QPushButton(printService->getDisplayName(), this);
    this->printServiceLayout->insertWidget(0, button);
    connect(button, &QPushButton::clicked, this, [&](){
     // TODO: Instead of forcing people to use Preferences, we should add UI here to select file format
      this->textBrowser->setHtml(printService->getInfoHtml());
      this->result = QString("PRINT_SERVICE:") + QString::fromStdString(key);
      this->okButton->setEnabled(true);
      LOG(this->result.toStdString());
    });
  }
  //  TODO: What if no services are available, print a warning?
}

void PrintInitDialog::on_octoPrintButton_clicked()
{
  // TODO: Instead of forcing people to use Preferences, we should add UI here to select file format
  this->textBrowser->setSource(QUrl{"qrc:/html/OctoPrintInfo.html"});
  this->result = "OCTOPRINT";
  this->okButton->setEnabled(true);
}

void PrintInitDialog::on_LocalSlicerButton_clicked()
{
  // TODO: Instead of forcing people to use Preferences, we should add UI here to:
  // 1. Select file format
  // 2. Select external program
  this->textBrowser->setSource(QUrl{"qrc:/html/LocalSlicerInfo.html"});
  this->result = "LOCALSLICER";
  this->okButton->setEnabled(true);
}

void PrintInitDialog::on_okButton_clicked()
{
  if (this->checkBoxRememberSelection->isChecked()) {
    QSettingsCached settings;
    settings.setValue(QString::fromStdString(Settings::Settings::defaultPrintService.key()), this->result);
  }
  accept();
}

void PrintInitDialog::on_cancelButton_clicked()
{
  reject();
}

QString PrintInitDialog::getResult()
{
  // Get service from setting
  // If service is valid, return service, else show dialog

  const QSettingsCached settings;
  auto service = settings.value(QString::fromStdString(Settings::Settings::defaultPrintService.key())).toString();
  if (isValidPrintServiceKey(service.toStdString())) return service;

  auto printInitDialog = new PrintInitDialog();
  auto printInitResult = printInitDialog->exec();
  printInitDialog->deleteLater();
  if (printInitResult == QDialog::Rejected) {
    return "NONE"; // TODO: Use empty string?
  }

  return printInitDialog->result;
}

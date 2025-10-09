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

#include "gui/OctoPrintApiKeyDialog.h"

#include <QString>
#include <QCheckBox>
#include <QColor>
#include <QDialog>
#include <QColorDialog>
#include <QLineEdit>
#include <QSvgRenderer>
#include <QPalette>
#include <QPainter>

#include "OctoPrint.h"

OctoPrintApiKeyDialog::OctoPrintApiKeyDialog()
{
  setupUi(this);
  QObject::connect(&networkTimer, &QTimer::timeout, this, &OctoPrintApiKeyDialog::timeout);
  QObject::connect(&animationTimer, &QTimer::timeout, this, &OctoPrintApiKeyDialog::animationUpdate);

  this->iconOk = QIcon::fromTheme("chokusen-circle-checkmark");
  this->iconError = QIcon::fromTheme("chokusen-circle-error");
  this->iconWaiting = QIcon::fromTheme("chokusen-loading");
}

void OctoPrintApiKeyDialog::startRequest()
{
  OctoPrint octoPrint;
  this->token = octoPrint.requestApiKey();
  this->apiKey.clear();
  this->labelMessage->setText(_("API key created, waiting for approval in OctoPrint..."));
  networkTimer.setSingleShot(true);
  networkTimer.start(1000);
  animationTimer.setInterval(100);
  animationTimer.start();
  this->pushButtonOk->setEnabled(false);
  this->pushButtonRetry->setEnabled(false);
}

void OctoPrintApiKeyDialog::paintIcon(const QIcon& icon, const qreal rotation)
{
  QPalette palette;
  QImage image(this->labelIcon->width(), this->labelIcon->width(), QImage::Format_ARGB32);
  image.fill(0x000000ff);
  QPainter painter(&image);
  painter.translate(QPoint{this->labelIcon->width() / 2, this->labelIcon->width() / 2});
  painter.rotate(rotation);
  painter.translate(QPoint{-this->labelIcon->width() / 2, -this->labelIcon->width() / 2});
  icon.paint(&painter, image.rect());
  QPixmap pixmap = QPixmap::fromImage(image);
  this->labelIcon->setPixmap(QPixmap::fromImage(image));
}

void OctoPrintApiKeyDialog::timeout()
{
  OctoPrint octoPrint;
  const auto [code, apiKey] = octoPrint.pollApiKeyApproval(this->token);
  switch (code) {
  case 200:
    this->token.clear();
    this->apiKey = apiKey;
    this->labelMessage->setText(_("API key approved."));
    animationTimer.stop();
    paintIcon(this->iconOk);
    this->pushButtonOk->setEnabled(true);
    break;
  case 202: networkTimer.start(1000); break;
  case 404:
  default:
    this->token.clear();
    this->apiKey.clear();
    this->labelMessage->setText(_("API key approval failed."));
    animationTimer.stop();
    paintIcon(this->iconError);
    this->pushButtonRetry->setEnabled(true);
    break;
  }
}

void OctoPrintApiKeyDialog::animationUpdate()
{
  static qreal rotation = 0.0;
  paintIcon(this->iconWaiting, rotation);
  rotation += 30;
}

int OctoPrintApiKeyDialog::exec()
{
  startRequest();
  return QDialog::exec();
}

void OctoPrintApiKeyDialog::on_pushButtonRetry_clicked() { startRequest(); }

void OctoPrintApiKeyDialog::on_pushButtonOk_clicked() { accept(); }

void OctoPrintApiKeyDialog::on_pushButtonCancel_clicked()
{
  this->token.clear();
  this->apiKey.clear();
  reject();
}

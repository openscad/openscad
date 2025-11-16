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

#include <QIcon>
#include <QTimer>
#include <QDialog>
#include <QString>

#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_OctoPrintApiKeyDialog.h"

class OctoPrintApiKeyDialog : public QDialog, public Ui::OctoPrintApiKeyDialog
{
  Q_OBJECT;

public:
  OctoPrintApiKeyDialog();

  int exec() override;
  const QString& getApiKey() const { return apiKey; }

private slots:
  void timeout();
  void animationUpdate();
  void on_pushButtonRetry_clicked();
  void on_pushButtonOk_clicked();
  void on_pushButtonCancel_clicked();

private:
  void startRequest();
  void paintIcon(const QIcon& icon, const qreal rotation = 0.0);

  QString token;
  QString apiKey;

  QTimer networkTimer;
  QTimer animationTimer;

  QIcon iconOk;
  QIcon iconError;
  QIcon iconWaiting;
};

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

#include <memory>
#include <QDialog>

#include "Settings.h"
#include "gui/qtgettext.h" // IWYU pragma: keep
#include "io/export.h"
#include "ui_Export3mfDialog.h"
#include "gui/InitConfigurator.h"

using SEBool = Settings::SettingsEntryBool;
using SEString = Settings::SettingsEntryString;

class Export3mfDialog : public QDialog, public Ui::Export3mfDialog, public InitConfigurator
{
  Q_OBJECT;

public:
  Export3mfDialog();

  int exec() override;

  std::shared_ptr<const Export3mfOptions> getOptions() const {
    return Export3mfOptions::fromSettings();
  }

private slots:
  void on_toolButtonColorsSelected_clicked();
  void on_toolButtonColorsSelectedReset_clicked();
  void on_toolButtonDecimalPrecisionReset_clicked();

private:
  void updateColor(const QColor& color);

  QColor color;
};

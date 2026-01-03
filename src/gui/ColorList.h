/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright The OpenSCAD Developers.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, see
 *  <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "gui/ColorList.h"

#include <QWidget>
#include <QString>
#include <QColor>

#include "gui/ColorLabel.h"
#include "gui/ColorLayout.h"
#include "gui/InitConfigurator.h"

class Ui_ColorListWidget;

class ColorList : public QWidget, public InitConfigurator
{
  Q_OBJECT
  static constexpr char SAMPLE_TEXT_DEFAULT[] = "Sample text... 0123456789!";

public:
  explicit ColorList(QWidget *parent = 0);
  ~ColorList() override;

private:
  void addColor(const QString& mapName, const QString& name, const QColor& color);
  void sortByName(const bool descending = false);
  void sortByColor(const bool descending = false);
  void sortByWarmth(const bool descending = false);
  void sortByLightness(const bool descending = false);
  void updateColorDialog(QColor& target);
  void updateSelectedColor();

protected slots:
  void updateSort();
  void updateFilter();
  void on_toolButtonAsForegroundReset_clicked();
  void on_toolButtonAsForegroundColorDialog_clicked();
  void on_toolButtonAsBackgroundReset_clicked();
  void on_toolButtonAsbackgroundColorDialog_clicked();
  void on_actionResetSampleTextForeground_triggered();
  void on_actionResetSampleTextBackground_triggered();
  void on_actionCopyColorName_triggered();
  void on_actionCopyColorRGB_triggered();

signals:
  void colorSelected(QString);

private:
  QColor asForeground;
  QColor asBackground;
  ColorLayout *layout;
  ColorLabel *selectedColorLabel;
  QList<QLayoutItem *> itemList;
  Ui_ColorListWidget *ui;
};

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

#include <QLabel>
#include <QString>
#include <QMouseEvent>

class ColorLabel : public QLabel
{
  Q_OBJECT

public:
  static constexpr char PROPERTY_COLOR_MAP_NAME[] = "ColorMapName";

  explicit ColorLabel(QWidget *parent = nullptr) : QLabel(parent) {}
  ~ColorLabel() = default;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

  void setColorInfo(const QString& message, const QColor& bgColor);
  void setSelected(const bool selected);
  const QColor& foregroundColor() const { return fgColor; }
  const QColor& backgroundColor() const { return bgColor; }

protected:
  void updateStyleSheet();
  QString calculateBorderColor(const QColor& ref, const bool selected);

signals:
  void clicked();

private:
  bool selected{};
  QColor fgColor;
  QColor bgColor;
  QPoint dragStartPosition;
};

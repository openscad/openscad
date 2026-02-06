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

#include <functional>

#include <QRect>
#include <QStyle>
#include <QLayout>

class ColorLayout : public QLayout
{
public:
  explicit ColorLayout(QWidget *parent, int margin = -1, int hs = -1, int vs = -1)
    : QLayout(parent), hs_(hs), vs_(vs)
  {
    setContentsMargins(margin, margin, margin, margin);
  }

  explicit ColorLayout(int margin = -1, int hs = -1, int vs = -1) : hs_(hs), vs_(vs)
  {
    setContentsMargins(margin, margin, margin, margin);
  }

  ~ColorLayout() {}

  void sort(const std::function<bool(const QWidget *w1, const QWidget *l2)>& func);

  void addItem(QLayoutItem *item) override { itemList.append(item); }

  int horizontalSpacing() const { return hs_; };

  int verticalSpacing() const { return vs_; }

  QLayoutItem *itemAt(int index) const override { return itemList.value(index); }

  QLayoutItem *takeAt(int index) override
  {
    if (index < 0 || index >= itemList.size()) {
      return nullptr;
    }
    return itemList.takeAt(index);
  }

  int count() const override { return itemList.size(); }

  Qt::Orientations expandingDirections() const override { return {}; }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int w) const override { return doLayout(QRect(0, 0, w, 0), true); }

  QSize sizeHint() const override { return minimumSize(); }

  void setGeometry(const QRect& rect) override
  {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
  }

  QSize minimumSize() const override;

private:
  int doLayout(const QRect& rect, bool testOnly) const;

private:
  int hs_;
  int vs_;
  QList<QLayoutItem *> itemList;
};

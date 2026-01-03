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
#include <algorithm>
#include <QtWidgets>

#include "gui/ColorLayout.h"

void ColorLayout::sort(const std::function<bool(const QWidget *l1, const QWidget *l2)>& func)
{
  std::sort(itemList.begin(), itemList.end(),
            [&func](QLayoutItem *i1, QLayoutItem *i2) { return func(i1->widget(), i2->widget()); });
  invalidate();
}

QSize ColorLayout::minimumSize() const
{
  QSize size;
  for (const auto item : itemList) {
    size = size.expandedTo(item->widget()->sizeHint());
  }

  const QMargins margins = contentsMargins();
  size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
  return size;
}

int ColorLayout::doLayout(const QRect& rect, bool testOnly) const
{
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();

  int maxw = 0;
  int maxh = 0;
  for (QLayoutItem *item : std::as_const(itemList)) {
    const auto w = item->widget();
    if (!w->isVisible()) {
      continue;
    }
    const auto sizeHint = w->sizeHint();
    if (sizeHint.width() > maxw) {
      maxw = sizeHint.width();
    }
    if (sizeHint.height() > maxh) {
      maxh = sizeHint.height();
    }
  }

  int columns =
    static_cast<int>(std::max(1.0f, std::floor(static_cast<float>(effectiveRect.width()) / maxw)));
  int columnWidth = static_cast<int>(std::floor(static_cast<float>(effectiveRect.width()) / columns));

  int idx = 0;
  int curY = top;
  for (QLayoutItem *item : std::as_const(itemList)) {
    const auto w = item->widget();
    const auto sizeHint = w->sizeHint();
    if (!w->isVisible()) {
      continue;
    }

    const int column = idx % columns;
    if (idx > 0 && column == 0) {
      curY += sizeHint.height();
    }

    const int effectiveColumnWidth =
      columns == 1 || column < columns - 1 ? columnWidth : effectiveRect.width() - column * columnWidth;

    const int wx = x + column * columnWidth;
    if (!testOnly) {
      const auto geometry = QRect(QPoint(wx, curY), QSize{effectiveColumnWidth, w->sizeHint().height()});
      item->setGeometry(geometry);
    }

    idx++;
  }

  return curY + maxh + bottom;
}

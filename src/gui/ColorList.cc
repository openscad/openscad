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

#include "gui/ColorList.h"

#include <cmath>
#include <functional>
#include <QPalette>
#include <QColorDialog>
#include <QClipboard>
#include <optional>

#include "core/ColorUtil.h"
#include "core/Settings.h"
#include "core/SettingsGuiEnums.h"
#include "gui/ColorLabel.h"
#include "gui/SettingsWriter.h"

#include "gui/qtgettext.h"
#include "ui_ColorList.h"

using S = Settings::SettingsColorList;

namespace {

// https://www.alanzucconi.com/2015/09/30/colour-sorting/
// https://stackoverflow.com/questions/3014402/sorting-a-list-of-colors-in-one-dimension
float get_color_sort_value(const QColor& color)
{
  const auto r = color.redF();
  const auto g = color.greenF();
  const auto b = color.blueF();

  const auto h = std::floor(8.0f * std::abs(color.hsvHueF()));
  const auto l = std::sqrt(0.299f * r * r + 0.587f * g * g + 0.114f * b * b);
  const auto c = 100.0f * h + 10.0f * l;

  return c;
}

// Based on examples shown at https://onlinepngtools.com/sort-colors
float get_warmth_sort_value(const QColor& color)
{
  qreal h, s, l;
  color.toHsl().getHslF(&h, &s, &l);

  const auto s1 = s < 0.2 ? 1.0 : 0.0;  // gray to the end
  const auto h1 = std::floor(8.0f * h) * (1.0 - s1);
  const auto l1 = static_cast<int>(h1) % 2 == 0 ? l : 1 - l;
  const auto c = 1000000 * s1 + 1000 * h1 + l1;

  return c;
}

float get_lightness_sort_value(const QColor& color)
{
  const auto h = std::abs(color.hsvHueF());
  const auto l = color.lightnessF();
  const auto c = 100.0f * l + 20.0f * h;

  return c;
}

}  // namespace

ColorList::ColorList(QWidget *parent) : QWidget(parent), ui(new Ui_ColorListWidget)
{
  ui->setupUi(this);
  ui->lineEditColorNameSelected->addAction(ui->actionCopyColorName, QLineEdit::TrailingPosition);
  ui->lineEditColorRgbSelected->addAction(ui->actionCopyColorRGB, QLineEdit::TrailingPosition);
  ui->lineEditAsForeground->addAction(ui->actionResetSampleTextForeground, QLineEdit::TrailingPosition);
  ui->lineEditAsBackground->addAction(ui->actionResetSampleTextBackground, QLineEdit::TrailingPosition);
  ui->actionResetSampleTextForeground->trigger();
  ui->actionResetSampleTextBackground->trigger();

  selectedColorLabel = nullptr;
  layout = new ColorLayout(ui->scrollAreaWidgetContentsColorList);

  for (const auto& colorMapEntry : OpenSCAD::getColorMaps()) {
    const auto mapName = QString::fromStdString(colorMapEntry.first);
    for (const auto& entry : colorMapEntry.second) {
      int r, g, b, a;
      const auto& name = QString::fromStdString(entry.first);
      if (entry.second.getRgba(r, g, b, a)) {
        if (a == 255) {
          const auto& color = QColor(r, g, b);
          addColor(mapName, name, color);
        }
      }
    }
  }

  // First initialize the settings without the update signal wired yet
  ui->checkBoxWebColors->setChecked(S::colorListWebColors.value());
  ui->checkBoxXkcdColors->setChecked(S::colorListXkcdColors.value());
  ui->radioButtonSortAscending->setChecked(S::colorListSortAscending.value());
  ui->radioButtonSortDescending->setChecked(!S::colorListSortAscending.value());
  initComboBox(ui->comboBoxFilterType, S::colorListFilterType);
  initComboBox(ui->comboBoxSortType, S::colorListSortType);

  on_toolButtonAsForegroundReset_clicked();
  on_toolButtonAsBackgroundReset_clicked();
  updateFilter();
  updateSort();

  // Connect signals last so following GUI changes are tracked in settings
  QObject::connect(ui->lineEditColorName, &QLineEdit::textChanged, this, &ColorList::updateFilter);
  QObject::connect(ui->checkBoxWebColors, &QCheckBox::toggled, this, &ColorList::updateFilter);
  QObject::connect(ui->checkBoxXkcdColors, &QCheckBox::toggled, this, &ColorList::updateFilter);
  QObject::connect(ui->comboBoxFilterType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &ColorList::updateFilter);
  QObject::connect(ui->radioButtonSortAscending, &QRadioButton::toggled, this, &ColorList::updateSort);
  QObject::connect(ui->radioButtonSortDescending, &QRadioButton::toggled, this, &ColorList::updateSort);
  QObject::connect(ui->comboBoxSortType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &ColorList::updateSort);
}

ColorList::~ColorList() { delete ui; }

void ColorList::updateSort()
{
  const bool descending = ui->radioButtonSortDescending->isChecked();
  const auto optSortType = getComboBoxValue(ui->comboBoxSortType, S::colorListSortType);
  const auto sortType = optSortType.value_or(S::colorListSortType.defaultValue());
  switch (sortType) {
  case ColorListSortType::alphabetical:    sortByName(descending); break;
  case ColorListSortType::by_color:        sortByColor(descending); break;
  case ColorListSortType::by_color_warmth: sortByWarmth(descending); break;
  case ColorListSortType::by_lightness:    sortByLightness(descending); break;
  default:                                 break;
  }
  S::colorListSortAscending.setValue(ui->radioButtonSortAscending->isChecked());
  S::colorListSortType.setValue(sortType);
  Settings::Settings::visit(SettingsWriter());
}

void ColorList::updateFilter()
{
  std::function<bool(const QString& name)> filterFunction;

  const QString& text = ui->lineEditColorName->text();
  const auto optFilterType = getComboBoxValue(ui->comboBoxFilterType, S::colorListFilterType);
  const auto filterType = optFilterType.value_or(S::colorListFilterType.defaultValue());
  switch (filterType) {
  case ColorListFilterType::fixed: {
    filterFunction = [text](const QString& name) { return name.contains(text); };
  }
  case ColorListFilterType::wildcard: {
    const auto regExp = QRegularExpression::wildcardToRegularExpression(text);
    // QT6: Update to ignore-case + non-anchored
    filterFunction = [text](const QString& name) { return name.contains(text); };
  }
  default: {
    const auto regExp = QRegularExpression(text, QRegularExpression::CaseInsensitiveOption);
    filterFunction = [text, regExp](const QString& name) { return name.contains(regExp); };
  }
  }

  QSet<QString> enabledMaps;
  if (ui->checkBoxWebColors->isChecked()) {
    enabledMaps.insert(OpenSCAD::COLOR_MAP_NAME_WEB_COLORS);
  }
  if (ui->checkBoxXkcdColors->isChecked()) {
    enabledMaps.insert(OpenSCAD::COLOR_MAP_NAME_XKCD_COLORS);
  }

  int count = 0;
  ui->scrollAreaWidgetContentsColorList->setUpdatesEnabled(false);
  for (const auto item : itemList) {
    const auto w = item->widget();
    const auto isMapEnabled =
      enabledMaps.contains(w->property(ColorLabel::PROPERTY_COLOR_MAP_NAME).toString());
    const auto visible = isMapEnabled && filterFunction(w->objectName());
    w->setVisible(visible);
    count += visible;
  }
  ui->scrollAreaWidgetContentsColorList->setUpdatesEnabled(true);
  layout->invalidate();

  ui->groupBoxFilter->setTitle(QString(_("Filter (%1 colors found)")).arg(count));
  S::colorListWebColors.setValue(ui->checkBoxWebColors->isChecked());
  S::colorListXkcdColors.setValue(ui->checkBoxXkcdColors->isChecked());
  S::colorListFilterType.setValue(filterType);
  Settings::Settings::visit(SettingsWriter());
}

void ColorList::addColor(const QString& mapName, const QString& name, const QColor& color)
{
  auto label = new ColorLabel(ui->scrollAreaWidgetContentsColorList);
  label->setColorInfo(name, color);
  label->setProperty(ColorLabel::PROPERTY_COLOR_MAP_NAME, mapName);
  connect(label, &ColorLabel::clicked, [this, label]() {
    if (selectedColorLabel) selectedColorLabel->setSelected(false);
    if (selectedColorLabel == label) {
      selectedColorLabel = nullptr;
    } else {
      selectedColorLabel = label;
      selectedColorLabel->setSelected(true);
    }
    updateSelectedColor();
  });

  label->show();
  auto item = new QWidgetItem(label);
  itemList.append(item);
  layout->addItem(item);
}

void ColorList::sortByName(const bool descending)
{
  layout->sort([descending](const QWidget *w1, const QWidget *w2) {
    const auto l1 = qobject_cast<const QLabel *>(w1);
    const auto l2 = qobject_cast<const QLabel *>(w2);
    return descending ? l1->text() > l2->text() : l1->text() < l2->text();
  });
}

void ColorList::sortByColor(const bool descending)
{
  layout->sort([descending](const QWidget *w1, const QWidget *w2) {
    const auto label1 = qobject_cast<const ColorLabel *>(w1);
    const auto label2 = qobject_cast<const ColorLabel *>(w2);

    const auto c1 = get_color_sort_value(label1->backgroundColor());
    const auto c2 = get_color_sort_value(label2->backgroundColor());

    return descending ? c1 < c2 : c1 > c2;
  });
}

void ColorList::sortByWarmth(const bool descending)
{
  layout->sort([descending](const QWidget *w1, const QWidget *w2) {
    const auto label1 = qobject_cast<const ColorLabel *>(w1);
    const auto label2 = qobject_cast<const ColorLabel *>(w2);

    const auto c1 = get_warmth_sort_value(label1->backgroundColor());
    const auto c2 = get_warmth_sort_value(label2->backgroundColor());

    return descending ? c1 > c2 : c1 < c2;
  });
}

void ColorList::sortByLightness(const bool descending)
{
  layout->sort([descending](const QWidget *w1, const QWidget *w2) {
    const auto label1 = qobject_cast<const ColorLabel *>(w1);
    const auto label2 = qobject_cast<const ColorLabel *>(w2);

    const auto c1 = get_lightness_sort_value(label1->backgroundColor());
    const auto c2 = get_lightness_sort_value(label2->backgroundColor());

    return descending ? c1 < c2 : c1 > c2;
  });
}

void ColorList::updateSelectedColor()
{
  QPalette defaultPalette;
  const auto styleSheet = QString("QLineEdit { color: %1; background-color: %2; }");
  const auto name = selectedColorLabel ? selectedColorLabel->text() : "";
  const auto colorRgb = selectedColorLabel ? selectedColorLabel->backgroundColor().name() : "";
  const auto colorRgbR =
    selectedColorLabel ? QString::number(selectedColorLabel->backgroundColor().red()) : "";
  const auto colorRgbG =
    selectedColorLabel ? QString::number(selectedColorLabel->backgroundColor().green()) : "";
  const auto colorRgbB =
    selectedColorLabel ? QString::number(selectedColorLabel->backgroundColor().blue()) : "";
  const auto color =
    selectedColorLabel ? selectedColorLabel->backgroundColor() : defaultPalette.text().color();
  ui->lineEditColorNameSelected->setText(name);
  ui->lineEditColorRgbSelected->setText(colorRgb);
  ui->lineEditColorRgbSelectedR->setText(colorRgbR);
  ui->lineEditColorRgbSelectedG->setText(colorRgbG);
  ui->lineEditColorRgbSelectedB->setText(colorRgbB);
  ui->lineEditAsForeground->setStyleSheet(styleSheet.arg(color.name(), asForeground.name()));
  ui->lineEditAsBackground->setStyleSheet(styleSheet.arg(asBackground.name(), color.name()));
}

void ColorList::on_toolButtonAsForegroundReset_clicked()
{
  QPalette defaultPalette;
  asForeground = defaultPalette.base().color().toRgb();
  updateSelectedColor();
}

void ColorList::updateColorDialog(QColor& target)
{
  const auto selected = QColorDialog::getColor(target, this).toRgb();
  if (selected.isValid()) {
    target = selected;
    updateSelectedColor();
  }
}

void ColorList::on_toolButtonAsForegroundColorDialog_clicked() { updateColorDialog(asForeground); }

void ColorList::on_toolButtonAsBackgroundReset_clicked()
{
  QPalette defaultPalette;
  asBackground = defaultPalette.base().color().toRgb();
  updateSelectedColor();
}

void ColorList::on_toolButtonAsbackgroundColorDialog_clicked() { updateColorDialog(asBackground); }

void ColorList::on_actionResetSampleTextForeground_triggered()
{
  ui->lineEditAsForeground->setText(SAMPLE_TEXT_DEFAULT);
  ui->lineEditAsForeground->setCursorPosition(0);
}

void ColorList::on_actionResetSampleTextBackground_triggered()
{
  ui->lineEditAsBackground->setText(SAMPLE_TEXT_DEFAULT);
  ui->lineEditAsBackground->setCursorPosition(0);
}

void ColorList::on_actionCopyColorName_triggered()
{
  QApplication::clipboard()->setText(ui->lineEditColorNameSelected->text());
}

void ColorList::on_actionCopyColorRGB_triggered()
{
  QApplication::clipboard()->setText(ui->lineEditColorRgbSelected->text());
}

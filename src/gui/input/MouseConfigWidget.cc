/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2014 Clifford Wolf <clifford@clifford.at> and
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

#include "gui/input/MouseConfigWidget.h"

#include <QChar>
#include <QComboBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <cstddef>
#include "core/Settings.h"
#include "gui/input/InputDriverManager.h"
#include "gui/SettingsWriter.h"
#include "gui/IgnoreWheelWhenNotFocused.h"
#include "gui/input/InputEventMapper.h"

MouseConfigWidget::MouseConfigWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
}

void MouseConfigWidget::updateMouseState(int nr, bool pressed) const {
  QString style = pressed ? MouseConfigWidget::ActiveStyleString : MouseConfigWidget::EmptyString;

  auto label = this->findChild<QLabel *>(QString("labelInputMouse%1").arg(nr));
  if (label == nullptr) return;
  label->setStyleSheet(style);
}

void MouseConfigWidget::init() {
  // Init the preset box (special case)
  comboBoxPreset->clear();
  comboBoxPreset->addItem(QString::fromStdString("OpenSCAD"));
  comboBoxPreset->addItem(QString::fromStdString("Blender"));
  comboBoxPreset->addItem(QString::fromStdString("Custom"));
  comboBoxPreset->setCurrentIndex(0);

  initActionComboBox(comboBoxLeftClick, Settings::Settings::inputMouseLeftClick);
  initActionComboBox(comboBoxMiddleClick, Settings::Settings::inputMouseMiddleClick);
  initActionComboBox(comboBoxRightClick, Settings::Settings::inputMouseRightClick);
  initActionComboBox(comboBoxShiftLeftClick, Settings::Settings::inputMouseShiftLeftClick);
  initActionComboBox(comboBoxShiftMiddleClick, Settings::Settings::inputMouseShiftMiddleClick);
  initActionComboBox(comboBoxShiftRightClick, Settings::Settings::inputMouseShiftRightClick);
  initActionComboBox(comboBoxCtrlLeftClick, Settings::Settings::inputMouseCtrlLeftClick);
  initActionComboBox(comboBoxCtrlMiddleClick, Settings::Settings::inputMouseCtrlMiddleClick);
  initActionComboBox(comboBoxCtrlRightClick, Settings::Settings::inputMouseCtrlRightClick);

  installIgnoreWheelWhenNotFocused(this);

  initialized = true;
}

void MouseConfigWidget::updateToPresets(const std::string& presetName)
{

}

void MouseConfigWidget::on_comboBoxPreset_activated(int val)
{
  applyComboBox(comboBoxPreset, val, Settings::Settings::inputMousePreset);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxLeftClick_activated(int val)
{
  applyComboBox(comboBoxLeftClick, val, Settings::Settings::inputMouseLeftClick);
  // FIXME - is this still the appropriate SIGNAL to emit? Do a grep to see how it's used so far,
  //  and in particular what it triggers.
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxMiddleClick_activated(int val)
{
  applyComboBox(comboBoxMiddleClick, val, Settings::Settings::inputMouseMiddleClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxRightClick_activated(int val)
{
  applyComboBox(comboBoxRightClick, val, Settings::Settings::inputMouseRightClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxShiftLeftClick_activated(int val)
{
  applyComboBox(comboBoxShiftLeftClick, val, Settings::Settings::inputMouseShiftLeftClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxShiftMiddleClick_activated(int val)
{
  applyComboBox(comboBoxShiftMiddleClick, val, Settings::Settings::inputMouseShiftMiddleClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxShiftRightClick_activated(int val)
{
  applyComboBox(comboBoxShiftRightClick, val, Settings::Settings::inputMouseShiftRightClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxCtrlLeftClick_activated(int val)
{
  applyComboBox(comboBoxCtrlLeftClick, val, Settings::Settings::inputMouseCtrlLeftClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxCtrlMiddleClick_activated(int val)
{
  applyComboBox(comboBoxCtrlMiddleClick, val, Settings::Settings::inputMouseCtrlMiddleClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::on_comboBoxCtrlRightClick_activated(int val)
{
  applyComboBox(comboBoxCtrlRightClick, val, Settings::Settings::inputMouseCtrlRightClick);
  emit inputMappingChanged();
}

void MouseConfigWidget::applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntryString& entry)
{
  entry.setValue(comboBox->itemData(val).toString().toStdString());
  writeSettings();
}

void MouseConfigWidget::writeSettings()
{
  Settings::Settings::visit(SettingsWriter());
}

void MouseConfigWidget::initActionComboBox(QComboBox *comboBox, const Settings::SettingsEntryString& entry)
{
  comboBox->clear();

  // FIXME - does commenting out this actually cause any issues?
  // // Create an empty icon, so that all comboboxes have the same alignment
  // QPixmap map = QPixmap(16, 16);
  // map.fill(Qt::transparent);
  // const QIcon emptyIcon = QIcon(map);

  // FIXME - these are just a placeholder to test the GUI
  // I need to add non-geodesic rotation.
  // Should also add zoom.
  // FIXME - I should really have some central register of "viewport movements".
  comboBox->addItem(QString::fromStdString("None"));
  comboBox->addItem(QString::fromStdString("Rotate"));
  comboBox->addItem(QString::fromStdString("Pan"));
  comboBox->setCurrentIndex(0);
}

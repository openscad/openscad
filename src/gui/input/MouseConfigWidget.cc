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
#include "gui/input/MouseConfig.h"

#include <QChar>
#include <QComboBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <map>
#include "core/Settings.h"
#include "gui/input/InputDriverManager.h"
#include "gui/SettingsWriter.h"
#include "gui/IgnoreWheelWhenNotFocused.h"
#include "gui/Preferences.h"

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
  // Init the preset box, ensure that ordering is respected.
  comboBoxPreset->clear();
  for (int i=0; i < MouseConfig::NUM_PRESETS; i++) {
    comboBoxPreset->addItem(QString::fromStdString(
      MouseConfig::presetNames.at(static_cast<MouseConfig::Preset>(i))));
  }
  comboBoxPreset->setCurrentIndex(0);

  // Populate the maps
  actionToComboBox.clear();
  actionToComboBox.insert({MouseConfig::LEFT_CLICK, comboBoxLeftClick});
  actionToComboBox.insert({MouseConfig::MIDDLE_CLICK, comboBoxMiddleClick});
  actionToComboBox.insert({MouseConfig::RIGHT_CLICK, comboBoxRightClick});
  actionToComboBox.insert({MouseConfig::SHIFT_LEFT_CLICK, comboBoxShiftLeftClick});
  actionToComboBox.insert({MouseConfig::SHIFT_MIDDLE_CLICK, comboBoxShiftMiddleClick});
  actionToComboBox.insert({MouseConfig::SHIFT_RIGHT_CLICK, comboBoxShiftRightClick});
  actionToComboBox.insert({MouseConfig::CTRL_LEFT_CLICK, comboBoxCtrlLeftClick});
  actionToComboBox.insert({MouseConfig::CTRL_MIDDLE_CLICK, comboBoxCtrlMiddleClick});
  actionToComboBox.insert({MouseConfig::CTRL_RIGHT_CLICK, comboBoxCtrlRightClick});
  actionToSetting.clear();
  actionToSetting.insert({MouseConfig::LEFT_CLICK, Settings::Settings::inputMouseLeftClick});
  actionToSetting.insert({MouseConfig::MIDDLE_CLICK, Settings::Settings::inputMouseMiddleClick});
  actionToSetting.insert({MouseConfig::RIGHT_CLICK, Settings::Settings::inputMouseRightClick});
  actionToSetting.insert({MouseConfig::SHIFT_LEFT_CLICK, Settings::Settings::inputMouseShiftLeftClick});
  actionToSetting.insert({MouseConfig::SHIFT_MIDDLE_CLICK, Settings::Settings::inputMouseShiftMiddleClick});
  actionToSetting.insert({MouseConfig::SHIFT_RIGHT_CLICK, Settings::Settings::inputMouseShiftRightClick});
  actionToSetting.insert({MouseConfig::CTRL_LEFT_CLICK, Settings::Settings::inputMouseCtrlLeftClick});
  actionToSetting.insert({MouseConfig::CTRL_MIDDLE_CLICK, Settings::Settings::inputMouseCtrlMiddleClick});
  actionToSetting.insert({MouseConfig::CTRL_RIGHT_CLICK, Settings::Settings::inputMouseCtrlRightClick});

  for (int i=0; i < MouseConfig::NUM_MOUSE_ACTIONS; i++) {
    initActionComboBox(actionToComboBox.at(static_cast<MouseConfig::MouseAction>(i)));
  }
  updateAllToPreset(static_cast<MouseConfig::Preset>(0));
  disableBoxes(true);

  installIgnoreWheelWhenNotFocused(this);

  initialized = true;
}

void MouseConfigWidget::updateAllToPreset(MouseConfig::Preset preset)
{
  auto presetMapping = MouseConfig::presetSettings.at(preset);
  for (int i=0; i < MouseConfig::NUM_MOUSE_ACTIONS; i++) {
    auto mouseAction = static_cast<MouseConfig::MouseAction>(i);
    auto comboBox = actionToComboBox.at(mouseAction);
    if (presetMapping.count(mouseAction) > 0) {
      comboBox->setCurrentIndex(presetMapping.at(mouseAction));
    } else {
      comboBox->setCurrentIndex(MouseConfig::NONE);
    }
  }
}

void MouseConfigWidget::disableBoxes(bool disable)
{
  for (int i=0; i < MouseConfig::NUM_MOUSE_ACTIONS; i++) {
    auto comboBox = actionToComboBox.at(static_cast<MouseConfig::MouseAction>(i));
    comboBox->setDisabled(disable);
  }
}

void MouseConfigWidget::on_comboBoxPreset_activated(int val)
{
  auto preset = static_cast<MouseConfig::Preset>(val);

  if (preset == MouseConfig::CUSTOM) {
    disableBoxes(false);
  } else {
    // Update values of other fields to preset
    updateAllToPreset(preset);
    // Preset should render all fields non-editable
    disableBoxes(true);
  }

  //applyComboBox(comboBoxPreset, val, Settings::Settings::inputMousePreset);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxLeftClick_activated(int val)
{
  applyComboBox(comboBoxLeftClick, val, Settings::Settings::inputMouseLeftClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxMiddleClick_activated(int val)
{
  applyComboBox(comboBoxMiddleClick, val, Settings::Settings::inputMouseMiddleClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxRightClick_activated(int val)
{
  applyComboBox(comboBoxRightClick, val, Settings::Settings::inputMouseRightClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxShiftLeftClick_activated(int val)
{
  applyComboBox(comboBoxShiftLeftClick, val, Settings::Settings::inputMouseShiftLeftClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxShiftMiddleClick_activated(int val)
{
  applyComboBox(comboBoxShiftMiddleClick, val, Settings::Settings::inputMouseShiftMiddleClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxShiftRightClick_activated(int val)
{
  applyComboBox(comboBoxShiftRightClick, val, Settings::Settings::inputMouseShiftRightClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxCtrlLeftClick_activated(int val)
{
  applyComboBox(comboBoxCtrlLeftClick, val, Settings::Settings::inputMouseCtrlLeftClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxCtrlMiddleClick_activated(int val)
{
  applyComboBox(comboBoxCtrlMiddleClick, val, Settings::Settings::inputMouseCtrlMiddleClick);
  emit updateMouseActions();
}

void MouseConfigWidget::on_comboBoxCtrlRightClick_activated(int val)
{
  applyComboBox(comboBoxCtrlRightClick, val, Settings::Settings::inputMouseCtrlRightClick);
  emit updateMouseActions();
}

void MouseConfigWidget::applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntryInt& entry)
{
  auto viewAction = static_cast<MouseConfig::ViewAction>(val);
  entry.setValue(viewAction);
  writeSettings();
}

void MouseConfigWidget::writeSettings()
{
  Settings::Settings::visit(SettingsWriter());
}

void MouseConfigWidget::initActionComboBox(QComboBox *comboBox)
{
  comboBox->clear();
  for (int i=0; i < MouseConfig::NUM_VIEW_ACTIONS; i++) {
    auto view_action = static_cast<MouseConfig::ViewAction>(i);
    comboBox->addItem(QString::fromStdString(MouseConfig::viewActionNames.at(view_action)));
  }
  // Note that we don't set the current index on initialization - that will be done by another call.
}

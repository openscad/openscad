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

#include "gui/input/AxisConfigWidget.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QProgressBar>
#include <cmath>
#include <QWidget>
#include <cstddef>
#include <string>


#include "gui/Settings.h"
#include "gui/input/InputDriverManager.h"
#include "gui/SettingsWriter.h"
#include "gui/IgnoreWheelWhenNotFocused.h"
#include "gui/InitConfigurator.h"

AxisConfigWidget::AxisConfigWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
}

void AxisConfigWidget::AxesChanged(int nr, double val) const {
  auto *progressBar = this->findChild<QProgressBar *>(QString("progressBarAxis%1").arg(nr));
  if (progressBar == nullptr) return;

  int value = val * 100;
  progressBar->setValue(value); //set where the bar is

  //QProgressBar generates the shown string from the format string.
  //By setting a format string without a place holder,
  //we can set arbitrary text, like a custom formatted double.
  //(Note: QProgressBar internally works on int, so has no formatting for double values)
  //(Note: The text of a QProgressBar can not be set directly)
  QString s = QString::number(val, 'f', 2);
  progressBar->setFormat(s);

  auto *deadzone = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxDeadzone%1").arg(nr));
  if (deadzone) {
    bool active = deadzone->value() < std::abs(val);
    QString style;
    if (this->darkModeDetected) {
      style = active ? ProgressbarStyleDarkActive : ProgressbarStyleDark;
    } else {
      style = active ? ProgressbarStyleLightActive : ProgressbarStyleLight;
    }
    progressBar->setStyleSheet(style);
  }
}

void AxisConfigWidget::init() {
  connect(this->pushButtonAxisTrim, SIGNAL(clicked()), this, SLOT(on_AxisTrim()));
  connect(this->pushButtonAxisTrimReset, SIGNAL(clicked()), this, SLOT(on_AxisTrimReset()));
  connect(this->pushButtonUpdate, SIGNAL(clicked()), this, SLOT(updateStates()));

  initComboBox(this->comboBoxTranslationX, Settings::Settings::inputTranslationX);
  initComboBox(this->comboBoxTranslationY, Settings::Settings::inputTranslationY);
  initComboBox(this->comboBoxTranslationZ, Settings::Settings::inputTranslationZ);
  initComboBox(this->comboBoxTranslationXVPRel, Settings::Settings::inputTranslationXVPRel);
  initComboBox(this->comboBoxTranslationYVPRel, Settings::Settings::inputTranslationYVPRel);
  initComboBox(this->comboBoxTranslationZVPRel, Settings::Settings::inputTranslationZVPRel);
  initComboBox(this->comboBoxRotationX, Settings::Settings::inputRotateX);
  initComboBox(this->comboBoxRotationY, Settings::Settings::inputRotateY);
  initComboBox(this->comboBoxRotationZ, Settings::Settings::inputRotateZ);
  initComboBox(this->comboBoxRotationXVPRel, Settings::Settings::inputRotateXVPRel);
  initComboBox(this->comboBoxRotationYVPRel, Settings::Settings::inputRotateYVPRel);
  initComboBox(this->comboBoxRotationZVPRel, Settings::Settings::inputRotateZVPRel);
  initComboBox(this->comboBoxZoom, Settings::Settings::inputZoom);
  initComboBox(this->comboBoxZoom2, Settings::Settings::inputZoom2);

#ifdef ENABLE_HIDAPI
  this->checkBoxHIDAPI->setEnabled(true);
  this->checkBoxHIDAPI->setToolTip(HidApiInputDriverDescription);
#else
  this->checkBoxHIDAPI->setToolTip(HidApiInputDriverDescription + "\n\r" + NotEnabledDuringBuild);
#endif

#ifdef ENABLE_SPNAV
  this->checkBoxSpaceNav->setEnabled(true);
  this->checkBoxSpaceNav->setToolTip(SpaceNavInputDriverDescription);
#else
  this->checkBoxSpaceNav->setToolTip(SpaceNavInputDriverDescription + "\n\r" + NotEnabledDuringBuild);
#endif

#ifdef ENABLE_JOYSTICK
  this->checkBoxJoystick->setEnabled(true);
  this->checkBoxJoystick->setToolTip(JoystickInputDriverDescription);
#else
  this->checkBoxJoystick->setToolTip(JoystickInputDriverDescription + "\n\r" + NotEnabledDuringBuild);
#endif

#ifdef ENABLE_QGAMEPAD
  this->checkBoxQGamepad->setEnabled(true);
  this->checkBoxQGamepad->setToolTip(QGamepadInputDriverDescription);
#else
  this->checkBoxQGamepad->setToolTip(QGamepadInputDriverDescription + "\n\r" + NotEnabledDuringBuild);
#endif

#ifdef ENABLE_DBUS
  this->checkBoxDBus->setEnabled(true);
  this->checkBoxDBus->setToolTip(DBusInputDriverDescription);
#else
  this->checkBoxDBus->setToolTip(DBusInputDriverDescription + "\n\r" + NotEnabledDuringBuild);
#endif

  initUpdateCheckBox(this->checkBoxHIDAPI,   Settings::Settings::inputEnableDriverHIDAPI);
  initUpdateCheckBox(this->checkBoxSpaceNav, Settings::Settings::inputEnableDriverSPNAV);
  initUpdateCheckBox(this->checkBoxJoystick, Settings::Settings::inputEnableDriverJOYSTICK);
  initUpdateCheckBox(this->checkBoxQGamepad, Settings::Settings::inputEnableDriverQGAMEPAD);
  initUpdateCheckBox(this->checkBoxDBus,     Settings::Settings::inputEnableDriverDBUS);

  installIgnoreWheelWhenNotFocused(this);

  for (size_t i = 0; i < InputEventMapper::getMaxAxis(); ++i) {
    auto spinTrim = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxTrim%1").arg(i));
    if (spinTrim) {
      initUpdateDoubleSpinBox(spinTrim, Settings::Settings::axisTrim(i));
    }
    auto spinDeadZone = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxDeadzone%1").arg(i));
    if (spinDeadZone) {
      initUpdateDoubleSpinBox(spinDeadZone, Settings::Settings::axisDeadzone(i));
    }
  }

  initUpdateDoubleSpinBox(this->doubleSpinBoxTranslationGain, Settings::Settings::inputTranslationGain);
  initUpdateDoubleSpinBox(this->doubleSpinBoxTranslationVPRelGain, Settings::Settings::inputTranslationVPRelGain);
  initUpdateDoubleSpinBox(this->doubleSpinBoxRotateGain, Settings::Settings::inputRotateGain);
  initUpdateDoubleSpinBox(this->doubleSpinBoxRotateVPRelGain, Settings::Settings::inputRotateVPRelGain);
  initUpdateDoubleSpinBox(this->doubleSpinBoxZoomGain, Settings::Settings::inputZoomGain);

  //use a custom style for the axis indicators,
  //to prevent getting operating system specific
  //(potentially animated) ProgressBars
  int textLightness = this->progressBarAxis0->palette().text().color().lightness();
  this->darkModeDetected = textLightness > 165;
  QString style = (this->darkModeDetected) ? ProgressbarStyleDark : ProgressbarStyleLight;

  auto progressbars = this->findChildren<QProgressBar *>();
  for (auto progressbar : progressbars) {
    progressbar->setStyleSheet(style);
    progressbar->setAlignment(Qt::AlignCenter);
  }

  initialized = true;
}

void AxisConfigWidget::on_comboBoxTranslationX_activated(int val)
{
  applyComboBox(comboBoxTranslationX, val, Settings::Settings::inputTranslationX);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxTranslationY_activated(int val)
{
  applyComboBox(comboBoxTranslationY, val, Settings::Settings::inputTranslationY);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxTranslationZ_activated(int val)
{
  applyComboBox(comboBoxTranslationZ, val, Settings::Settings::inputTranslationZ);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxTranslationXVPRel_activated(int val)
{
  applyComboBox(comboBoxTranslationXVPRel, val, Settings::Settings::inputTranslationXVPRel);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxTranslationYVPRel_activated(int val)
{
  applyComboBox(comboBoxTranslationYVPRel, val, Settings::Settings::inputTranslationYVPRel);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxTranslationZVPRel_activated(int val)
{
  applyComboBox(comboBoxTranslationZVPRel, val, Settings::Settings::inputTranslationZVPRel);
  emit inputMappingChanged();
}
void AxisConfigWidget::on_comboBoxRotationX_activated(int val)
{
  applyComboBox(comboBoxRotationX, val, Settings::Settings::inputRotateX);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxRotationY_activated(int val)
{
  applyComboBox(comboBoxRotationY, val, Settings::Settings::inputRotateY);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxRotationZ_activated(int val)
{
  applyComboBox(comboBoxRotationZ, val, Settings::Settings::inputRotateZ);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxRotationXVPRel_activated(int val)
{
  applyComboBox(comboBoxRotationXVPRel, val, Settings::Settings::inputRotateXVPRel);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxRotationYVPRel_activated(int val)
{
  applyComboBox(comboBoxRotationYVPRel, val, Settings::Settings::inputRotateYVPRel);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxRotationZVPRel_activated(int val)
{
  applyComboBox(comboBoxRotationZVPRel, val, Settings::Settings::inputRotateZVPRel);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxZoom_activated(int val)
{
  applyComboBox(comboBoxZoom, val, Settings::Settings::inputZoom);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_comboBoxZoom2_activated(int val)
{
  applyComboBox(comboBoxZoom2, val, Settings::Settings::inputZoom2);
  emit inputMappingChanged();
}

void AxisConfigWidget::on_doubleSpinBoxTrim0_valueChanged(double val)
{
  Settings::Settings::axisTrim0.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim1_valueChanged(double val)
{
  Settings::Settings::axisTrim1.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim2_valueChanged(double val)
{
  Settings::Settings::axisTrim2.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim3_valueChanged(double val)
{
  Settings::Settings::axisTrim3.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim4_valueChanged(double val)
{
  Settings::Settings::axisTrim4.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim5_valueChanged(double val)
{
  Settings::Settings::axisTrim5.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim6_valueChanged(double val)
{
  Settings::Settings::axisTrim6.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim7_valueChanged(double val)
{
  Settings::Settings::axisTrim7.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim8_valueChanged(double val)
{
  Settings::Settings::axisTrim8.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone0_valueChanged(double val)
{
  Settings::Settings::axisDeadzone0.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone1_valueChanged(double val)
{
  Settings::Settings::axisDeadzone1.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone2_valueChanged(double val)
{
  Settings::Settings::axisDeadzone2.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone3_valueChanged(double val)
{
  Settings::Settings::axisDeadzone3.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone4_valueChanged(double val)
{
  Settings::Settings::axisDeadzone4.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone5_valueChanged(double val)
{
  Settings::Settings::axisDeadzone5.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone6_valueChanged(double val)
{
  Settings::Settings::axisDeadzone6.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone7_valueChanged(double val)
{
  Settings::Settings::axisDeadzone7.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone8_valueChanged(double val)
{
  Settings::Settings::axisDeadzone8.setValue(val);
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxRotateGain_valueChanged(double val)
{
  Settings::Settings::inputRotateGain.setValue(val);
  emit inputGainChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxRotateVPRelGain_valueChanged(double val)
{
  Settings::Settings::inputRotateVPRelGain.setValue(val);
  emit inputGainChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTranslationGain_valueChanged(double val)
{
  Settings::Settings::inputTranslationGain.setValue(val);
  emit inputGainChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTranslationVPRelGain_valueChanged(double val)
{
  Settings::Settings::inputTranslationVPRelGain.setValue(val);
  emit inputGainChanged();
  writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxZoomGain_valueChanged(double val)
{
  Settings::Settings::inputZoomGain.setValue(val);
  emit inputGainChanged();
  writeSettings();
}

void AxisConfigWidget::on_AxisTrim()
{
  InputEventMapper::instance()->onAxisAutoTrim();

  for (size_t i = 0; i < InputEventMapper::getMaxAxis(); ++i) {
    auto spin = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxTrim%1").arg(i));
    if (spin) {
      spin->setValue(Settings::Settings::axisTrim(i).value());
    }
  }
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_AxisTrimReset()
{
  InputEventMapper::instance()->onAxisTrimReset();
  for (size_t i = 0; i < InputEventMapper::getMaxAxis(); ++i) {
    auto spin = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxTrim%1").arg(i));
    if (spin) {
      Settings::Settings::axisTrim(i).setValue(0.00);
      spin->setValue(0.00);
    }
  }
  emit inputCalibrationChanged();
  writeSettings();
}

void AxisConfigWidget::on_checkBoxHIDAPI_toggled(bool val)
{
  if (initialized) {
    Settings::Settings::inputEnableDriverHIDAPI.setValue(val);
    writeSettings();

    QFont font;
    font.setItalic(true);
    checkBoxHIDAPI->setFont(font);
  }
}

void AxisConfigWidget::on_checkBoxSpaceNav_toggled(bool val)
{
  if (initialized) {
    Settings::Settings::inputEnableDriverSPNAV.setValue(val);
    writeSettings();
    QFont font;
    font.setItalic(true);
    checkBoxSpaceNav->setFont(font);
  }
}

void AxisConfigWidget::on_checkBoxJoystick_toggled(bool val)
{
  if (initialized) {
    Settings::Settings::inputEnableDriverJOYSTICK.setValue(val);
    writeSettings();
    QFont font;
    font.setItalic(true);
    checkBoxJoystick->setFont(font);
  }
}

void AxisConfigWidget::on_checkBoxQGamepad_toggled(bool val)
{
  if (initialized) {
    Settings::Settings::inputEnableDriverQGAMEPAD.setValue(val);
    writeSettings();
    QFont font;
    font.setItalic(true);
    checkBoxQGamepad->setFont(font);
  }
}

void AxisConfigWidget::on_checkBoxDBus_toggled(bool val)
{
  if (initialized) {
    Settings::Settings::inputEnableDriverDBUS.setValue(val);
    writeSettings();
    QFont font;
    font.setItalic(true);
    checkBoxDBus->setFont(font);
  }
}

void AxisConfigWidget::applyComboBox(QComboBox * /*comboBox*/, int val, Settings::SettingsEntryEnum& entry)
{
  entry.setIndex(val);
  writeSettings();
}

void AxisConfigWidget::writeSettings()
{
  Settings::Settings::visit(SettingsWriter());
}


void AxisConfigWidget::updateStates(){
  if (!initialized) return;

  size_t cnt = InputDriverManager::instance()->getAxisCount();
  for (size_t i = 0; i < InputEventMapper::getMaxAxis(); ++i) {
    auto progressbar = this->findChild<QProgressBar *>(QString("progressBarAxis%1").arg(i));
    if (progressbar) {
      if (cnt <= i) {
        progressbar->setEnabled(false);
        progressbar->setMinimum(0);
      } else {
        progressbar->setEnabled(true);
        progressbar->setMinimum(-100);
      }
    }
  }

  auto manager = InputDriverManager::instance();
  std::string infos = manager->listDriverInfos();
  label_driverInfo->setText(QString::fromStdString(infos));
}

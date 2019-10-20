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

#include <QWidget>
#include "AxisConfigWidget.h"

#include "settings.h"
#include "QSettingsCached.h"
#include "input/InputDriverManager.h"
#include "SettingsWriter.h"
#include "WheelIgnorer.h"

AxisConfigWidget::AxisConfigWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

AxisConfigWidget::~AxisConfigWidget()
{
}

void AxisConfigWidget::AxesChanged(int nr, double val) const{
	QProgressBar* progressBar = this->findChild<QProgressBar *>(QString("progressBarAxis%1").arg(nr));
	if(progressBar==nullptr) return;

	int value = val * 100;
	progressBar->setValue(value); //set where the bar is

	//QProgressBar generates the shown string from the format string.
	//By setting a format string without a place holder,
	//we can set arbitrary text, like a custom formatted double.
	//(Note: QProgressBar internally works on int, so has no formating for double values)
	//(Note: The text of a QProgressBar can not be set directly)
	QString s =  QString::number(val, 'f', 2 );
	progressBar->setFormat(s); 
	
	QDoubleSpinBox* deadzone = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxDeadzone%1").arg(nr));
	if(deadzone){
		bool active = deadzone->value() < std::abs(val);
		QString style;
		if(this->darkModeDetected){
			style = active ? ProgressbarStyleDarkActive : ProgressbarStyleDark;
		}else{
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
	this->checkBoxQGamepad->setToolTip(QGamepadInputDriverDescription );
#else
	this->checkBoxQGamepad->setToolTip(QGamepadInputDriverDescription  + "\n\r" + NotEnabledDuringBuild);
#endif

#ifdef ENABLE_DBUS
	this->checkBoxDBus->setEnabled(true);
	this->checkBoxDBus->setToolTip(DBusInputDriverDescription);
#else
	this->checkBoxDBus->setToolTip(DBusInputDriverDescription + "\n\r" + NotEnabledDuringBuild);
#endif

	initCheckBox(this->checkBoxHIDAPI,   Settings::Settings::inputEnableDriverHIDAPI);
	initCheckBox(this->checkBoxSpaceNav, Settings::Settings::inputEnableDriverSPNAV);
	initCheckBox(this->checkBoxJoystick, Settings::Settings::inputEnableDriverJOYSTICK);
	initCheckBox(this->checkBoxQGamepad, Settings::Settings::inputEnableDriverQGAMEPAD);
	initCheckBox(this->checkBoxDBus,     Settings::Settings::inputEnableDriverDBUS);

	auto *wheelIgnorer = new WheelIgnorer(this);
	auto comboBoxes = this->findChildren<QComboBox *>();
	for (auto comboBox : comboBoxes) {
		comboBox->installEventFilter(wheelIgnorer);
	}

	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		std::string s = std::to_string(i);

		auto spin = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxTrim%1").arg(i));
		auto ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s);
		if(spin && ent){
			initDoubleSpinBox(spin,*ent);
		}
		spin = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxDeadzone%1").arg(i));
		ent = Settings::Settings::inst()->getSettingEntryByName("axisDeadzone" +s);
		if(spin && ent){
			initDoubleSpinBox(spin,*ent);
		}
	}

	initDoubleSpinBox(this->doubleSpinBoxTranslationGain, Settings::Settings::inputTranslationGain);
	initDoubleSpinBox(this->doubleSpinBoxTranslationVPRelGain, Settings::Settings::inputTranslationVPRelGain);
	initDoubleSpinBox(this->doubleSpinBoxRotateGain, Settings::Settings::inputRotateGain);
	initDoubleSpinBox(this->doubleSpinBoxZoomGain, Settings::Settings::inputZoomGain);

	//use a custom style for the axis indicators,
	//to prevent getting operating system specific
	//(potentially animated) ProgressBars
	int textLightness = this->progressBarAxis0->palette().text().color().lightness();
	this->darkModeDetected = textLightness>165;
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
	Settings::Settings::inst()->set(Settings::Settings::axisTrim0, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim1_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim1, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim2_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim2, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim3_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim3, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim4_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim4, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim5_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim5, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim6_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim6, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim7_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim7, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTrim8_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim8, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone0_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone0, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone1_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone1, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone2_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone2, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone3_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone3, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone4_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone4, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone5_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone5, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone6_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone6, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone7_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone7, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxDeadzone8_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone8, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxRotateGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputRotateGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTranslationGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputTranslationGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxTranslationVPRelGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputTranslationVPRelGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void AxisConfigWidget::on_doubleSpinBoxZoomGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputZoomGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void AxisConfigWidget::on_AxisTrim()
{
	InputEventMapper::instance()->onAxisAutoTrim();

	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		std::string s = std::to_string(i);

		auto spin = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxTrim%1").arg(i));
		auto ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s);

		if(spin && ent){
			spin->setValue((double)Settings::Settings::inst()->get(*ent).toDouble());
		}
	}
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_AxisTrimReset()
{
	InputEventMapper::instance()->onAxisTrimReset();
	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		std::string s = std::to_string(i);

		auto ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s);
		if(ent){
			Settings::Settings::inst()->set(*ent, Value(0.00));
		}

		auto spin = this->findChild<QDoubleSpinBox *>(QString("doubleSpinBoxTrim%1").arg(i));
		if(spin){
			spin->setValue(0.00);
		}
	}
	emit inputCalibrationChanged();
	writeSettings();
}

void AxisConfigWidget::on_checkBoxHIDAPI_toggled(bool val)
{
	if(initialized){
		Settings::Settings::inst()->set(Settings::Settings::inputEnableDriverHIDAPI, Value(val));
		writeSettings();

		QFont font;
		font.setItalic(true);
		checkBoxHIDAPI->setFont(font);
	}
}

void AxisConfigWidget::on_checkBoxSpaceNav_toggled(bool val)
{
	if(initialized){
		Settings::Settings::inst()->set(Settings::Settings::inputEnableDriverSPNAV, Value(val));
		writeSettings();
		QFont font;
		font.setItalic(true);
		checkBoxSpaceNav->setFont(font);
	}
}

void AxisConfigWidget::on_checkBoxJoystick_toggled(bool val)
{
	if(initialized){
		Settings::Settings::inst()->set(Settings::Settings::inputEnableDriverJOYSTICK, Value(val));
		writeSettings();
		QFont font;
		font.setItalic(true);
		checkBoxJoystick->setFont(font);
	}
}

void AxisConfigWidget::on_checkBoxQGamepad_toggled(bool val)
{
	if(initialized){
		Settings::Settings::inst()->set(Settings::Settings::inputEnableDriverQGAMEPAD, Value(val));
		writeSettings();
		QFont font;
		font.setItalic(true);
		checkBoxQGamepad->setFont(font);
	}
}

void AxisConfigWidget::on_checkBoxDBus_toggled(bool val)
{
	if(initialized){
		Settings::Settings::inst()->set(Settings::Settings::inputEnableDriverDBUS, Value(val));
		writeSettings();
		QFont font;
		font.setItalic(true);
		checkBoxDBus->setFont(font);
	}
}

void AxisConfigWidget::applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntry& entry)
{
	QString s = comboBox->itemData(val).toString();
	Settings::Settings::inst()->set(entry, Value(s.toStdString()));
	writeSettings();
}

void AxisConfigWidget::initSpinBox(QSpinBox *spinBox, const Settings::SettingsEntry& entry)
{
	const RangeType &range = entry.range().toRange();
	spinBox->setMinimum(range.begin_value());
	spinBox->setMaximum(range.end_value());
}

void AxisConfigWidget::initDoubleSpinBox(QDoubleSpinBox *spinBox, const Settings::SettingsEntry& entry)
{
	Settings::Settings *s = Settings::Settings::inst();

	const RangeType &range = entry.range().toRange();
	spinBox->blockSignals(true);
	spinBox->setMinimum(range.begin_value());
	spinBox->setSingleStep(range.step_value());
	spinBox->setMaximum(range.end_value());
	spinBox->setValue((double)s->get(entry).toDouble());
	spinBox->blockSignals(false);
}

void AxisConfigWidget::initCheckBox(QCheckBox *checkBox, const Settings::SettingsEntry& entry)
{
	Settings::Settings *s = Settings::Settings::inst();
	const Value &value = s->get(entry);
	bool state = value.toBool();

	checkBox->blockSignals(true);
	checkBox->setChecked(state);
	checkBox->blockSignals(false);
}

void AxisConfigWidget::updateComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry)
{
	Settings::Settings *s = Settings::Settings::inst();

	const Value &value = s->get(entry);
	QString text = QString::fromStdString(value.toString());
	int idx = comboBox->findData(text);
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	} else {
		Value defaultValue = entry.defaultValue();
		QString defaultText = QString::fromStdString(defaultValue.toString());
		int defIdx = comboBox->findData(defaultText);
		if (defIdx >= 0) {
			comboBox->setCurrentIndex(defIdx);
		} else {
			comboBox->setCurrentIndex(0);
		}
	}
}

void AxisConfigWidget::writeSettings()
{
	SettingsWriter settingsWriter;
	Settings::Settings::inst()->visit(settingsWriter);
}

void AxisConfigWidget::initComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry)
{
	comboBox->clear();
	// Range is a vector of 2D vectors: [[name, value], ...]
	for(const auto &v : *entry.range().toVectorPtr()) {
		QString val = QString::fromStdString(v[0].toString());
		QString qtext = QString::fromStdString(gettext(v[1].toString().c_str()));
		comboBox->addItem(qtext, val);
	}
	updateComboBox(comboBox, entry);
}

void AxisConfigWidget::updateStates(){
	if(!initialized) return;

	int cnt = InputDriverManager::instance()->getAxisCount();
	for (int i=0;i<InputEventMapper::getMaxAxis();i++) {
		auto progressbar = this->findChild<QProgressBar *>(QString("progressBarAxis%1").arg(i));
		if( cnt <= i){
			progressbar->setEnabled(false);
			progressbar->setMinimum(0);
		}else{
			progressbar->setEnabled(true);
			progressbar->setMinimum(-100);
		}
	}

	auto manager = InputDriverManager::instance();
	std::string infos = manager->listDriverInfos();
	label_driverInfo->setText(QString::fromStdString(infos));
}

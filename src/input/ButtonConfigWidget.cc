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
#include "ButtonConfigWidget.h"
#include "settings.h"
#include "QSettingsCached.h"
#include "input/InputDriverManager.h"
#include "SettingsWriter.h"
#include "WheelIgnorer.h"

ButtonConfigWidget::ButtonConfigWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

ButtonConfigWidget::~ButtonConfigWidget()
{
}

void ButtonConfigWidget::updateButtonState(int nr, bool pressed) const{
	QString style = pressed ? ButtonConfigWidget::ActiveStyleString : ButtonConfigWidget::EmptyString;
	std::string number = std::to_string(nr);

	auto label = this->findChild<QLabel *>(QString("labelInputButton%1").arg(nr));
	if(label==nullptr) return;
	label->setStyleSheet(style);
}

void ButtonConfigWidget::init() {
	for (int i = 0; i < InputEventMapper::getMaxButtons(); ++i ){
		std::string s = std::to_string(i);
		auto box = this->findChild<QComboBox *>(QString("comboBoxButton%1").arg(i));
		auto ent = Settings::Settings::inst()->getSettingEntryByName("button" +s);
		if(box && ent){
			initComboBox(box,*ent);
		}
	}

	auto wheelIgnorer = new WheelIgnorer(this);
	auto comboBoxes = this->findChildren<QComboBox *>();
	for (auto comboBox : comboBoxes) {
		comboBox->installEventFilter(wheelIgnorer);
	}

	initialized = true;
}

void ButtonConfigWidget::on_comboBoxButton0_activated(int val)
{
	applyComboBox(comboBoxButton0, val, Settings::Settings::inputButton0);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton1_activated(int val)
{
	applyComboBox(comboBoxButton1, val, Settings::Settings::inputButton1);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton2_activated(int val)
{
	applyComboBox(comboBoxButton2, val, Settings::Settings::inputButton2);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton3_activated(int val)
{
	applyComboBox(comboBoxButton3, val, Settings::Settings::inputButton3);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton4_activated(int val)
{
	applyComboBox(comboBoxButton4, val, Settings::Settings::inputButton4);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton5_activated(int val)
{
	applyComboBox(comboBoxButton5, val, Settings::Settings::inputButton5);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton6_activated(int val)
{
	applyComboBox(comboBoxButton6, val, Settings::Settings::inputButton6);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton7_activated(int val)
{
	applyComboBox(comboBoxButton7, val, Settings::Settings::inputButton7);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton8_activated(int val)
{
	applyComboBox(comboBoxButton8, val, Settings::Settings::inputButton8);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton9_activated(int val)
{
	applyComboBox(comboBoxButton9, val, Settings::Settings::inputButton9);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton10_activated(int val)
{
	applyComboBox(comboBoxButton10, val, Settings::Settings::inputButton10);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton11_activated(int val)
{
	applyComboBox(comboBoxButton11, val, Settings::Settings::inputButton11);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton12_activated(int val)
{
	applyComboBox(comboBoxButton12, val, Settings::Settings::inputButton12);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton13_activated(int val)
{
	applyComboBox(comboBoxButton13, val, Settings::Settings::inputButton13);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton14_activated(int val)
{
	applyComboBox(comboBoxButton14, val, Settings::Settings::inputButton14);
        emit inputMappingChanged();
}

void ButtonConfigWidget::on_comboBoxButton15_activated(int val)
{
	applyComboBox(comboBoxButton15, val, Settings::Settings::inputButton15);
        emit inputMappingChanged();
}

void ButtonConfigWidget::applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntry& entry)
{
	QString s = comboBox->itemData(val).toString();
	Settings::Settings::inst()->set(entry, Value(s.toStdString()));
	writeSettings();
}

void ButtonConfigWidget::updateComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry)
{
	Settings::Settings *s = Settings::Settings::inst();

	const Value &value = s->get(entry);
	QString text = QString::fromStdString(value.toString());
	int idx = comboBox->findData(text);
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	} else {
		comboBox->addItem(QIcon::fromTheme("emblem-unreadable"), text + " " + _("(not supported)"), text);
		int defIdx = comboBox->findData(text);
		if (defIdx >= 0) {
			comboBox->setCurrentIndex(defIdx);
		} else {
			comboBox->setCurrentIndex(0);
		}
	}
}

void ButtonConfigWidget::writeSettings()
{
	SettingsWriter settingsWriter;
	Settings::Settings::inst()->visit(settingsWriter);
}

void ButtonConfigWidget::initComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry)
{
	comboBox->clear();

	// Create an empty icon, so that all comboboxes have the same alignment
	QPixmap map = QPixmap(16, 16);
	map.fill(Qt::transparent);
	const QIcon emptyIcon = QIcon(map);

	for (const auto &v : entry.range().toVector()) {
		const auto icon = emptyIcon;
		const auto desc = QString::fromStdString(gettext(v[1].toString().c_str()));
		const auto actionName = QString::fromStdString(v[0].toString());
		comboBox->addItem(icon, desc, actionName);
	}

	for (const auto &action : InputDriverManager::instance()->getActions()) {
		const auto icon = action.icon;
		const auto effectiveIcon = icon.isNull() ? emptyIcon : icon;
		const auto desc = QString(action.description).remove(QChar('&'));
		comboBox->addItem(effectiveIcon, desc, action.name);
	}

	updateComboBox(comboBox, entry);
}

void ButtonConfigWidget::updateStates(){
	if(!initialized) return;

	int cnt = InputDriverManager::instance()->getButtonCount();
	for (int i=0; i<InputEventMapper::getMaxButtons(); ++i) {
		auto label = this->findChild<QLabel *>(QString("labelInputButton%1").arg(i));
		QString style =(cnt <= i) ? ButtonConfigWidget::DisabledStyleString : ButtonConfigWidget::EmptyString;
		label->setStyleSheet(style);
	}
}

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
#include "Preferences.h"
#include <QWidget>
#include "ButtonConfigWidget.h"

void ButtonConfigWidget::updateButtonState(int nr, bool pressed) const{
	QString Style = Preferences::EmptyString;
	if(pressed){
		Style=Preferences::ActiveStyleString;
	}
	std::string number = std::to_string(nr);

	QLabel* label = this->findChild<QLabel *>(QString::fromStdString("labelInputButton"+number));
	if(label==0) return;
	label->setStyleSheet(Style);
}

void ButtonConfigWidget::init() {
		for (int i = 0; i < InputEventMapper::getMaxButtons(); i++ ){
			std::string s = std::to_string(i);
			QComboBox* box = this->centralwidget->findChild<QComboBox *>(QString::fromStdString("comboBoxButton"+s));
			Settings::SettingsEntry* ent = Settings::Settings::inst()->getSettingEntryByName("button" +s );
			if(box && ent){
				initComboBox(box,*ent);
			}
		}
		
	for (int i = 0; i < InputEventMapper::getMaxButtons(); i++ ){
		std::string s = std::to_string(i);
		QComboBox* box = this->centralwidget->findChild<QComboBox *>(QString::fromStdString("comboBoxButton"+s));
		Settings::SettingsEntry* ent = Settings::Settings::inst()->getSettingEntryByName("button" +s );
		if(box && ent){
			updateComboBox(box,*ent);
		}
	}

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

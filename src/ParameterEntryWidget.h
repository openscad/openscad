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
#pragma once

#include "value.h"
#include "qtgettext.h"
#include "ui_ParameterEntryWidget.h"

class ParameterEntryWidget : public QWidget, public Ui::ParameterEntryWidget
{
	Q_OBJECT

        typedef enum { UNDEFINED, COMBOBOX, SLIDER, CHECKBOX, TEXT, NUMBER, VECTOR } parameter_type_t;

        ValuePtr value;
        ValuePtr values;
        ValuePtr defaultValue;
	Value::ValueType vt;
	Value::ValueType dvt;
        parameter_type_t target;

public:
	ParameterEntryWidget(QWidget *parent = 0);
	virtual ~ParameterEntryWidget();

        ValuePtr getValue();
        bool isDefaultValue();
        void setAssignment(class Context *context, const class Assignment *assignment, const ValuePtr defaultValue);
        void applyParameter(class Assignment *assignment);

protected slots:
        void on_comboBox_activated(int);
        void on_slider_valueChanged(int);
        void on_lineEdit_editingFinished();
        void on_checkBox_toggled();
        void on_doubleSpinBox1_valueChanged(double);
        void on_doubleSpinBox2_valueChanged(double);
        void on_doubleSpinBox3_valueChanged(double);
 
signals:
        void changed();

protected:
        void updateVectorValue();
        void setName(const QString& name);
        void setValue(const class ValuePtr defaultValue, const class ValuePtr values);
        void setDescription(const QString& description);
};

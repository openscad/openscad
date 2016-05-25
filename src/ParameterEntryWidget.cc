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
#include "ParameterEntryWidget.h"

#include "value.h"
#include "typedefs.h"
#include "expression.h"

ParameterEntryWidget::ParameterEntryWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

ParameterEntryWidget::~ParameterEntryWidget()
{
}

void ParameterEntryWidget::on_comboBox_activated(int idx)
{
	const Value *v = (const Value *)comboBox->itemData(idx).value<void *>();
    object.value = ValuePtr(*v);
	emit changed();
}

void ParameterEntryWidget::on_slider_valueChanged(int)
{
	double v = slider->value();
    object.value = ValuePtr(v);
	this->labelSliderValue->setText(QString::number(v, 'f', 0));
	emit changed();
}

void ParameterEntryWidget::on_lineEdit_editingFinished()
{
    if (object.dvt == Value::NUMBER) {
		try {
            object.value = ValuePtr(boost::lexical_cast<double>(lineEdit->text().toStdString()));
		} catch (const boost::bad_lexical_cast& e) {
            lineEdit->setText(QString::fromStdString(object.defaultValue->toString()));
		}
	} else {
        object.value = ValuePtr(lineEdit->text().toStdString());
	}
	emit changed();
}

void ParameterEntryWidget::on_checkBox_toggled()
{
    object.value = ValuePtr(checkBox->isChecked());
	emit changed();
}

void ParameterEntryWidget::on_doubleSpinBox1_valueChanged(double)
{
	updateVectorValue();
}

void ParameterEntryWidget::on_doubleSpinBox2_valueChanged(double)
{
	updateVectorValue();
}

void ParameterEntryWidget::on_doubleSpinBox3_valueChanged(double)
{
	updateVectorValue();
}

void ParameterEntryWidget::updateVectorValue()
{
    if (object.target == 5) {
        object.value = ValuePtr(doubleSpinBox1->value());
	} else {
		Value::VectorType vt;

		vt.push_back(this->doubleSpinBox1->value());
		if (!this->doubleSpinBox2->isReadOnly()) {
			vt.push_back(this->doubleSpinBox2->value());
		}
		if (!this->doubleSpinBox3->isReadOnly()) {
			vt.push_back(this->doubleSpinBox3->value());
		}
		if (!this->doubleSpinBox4->isReadOnly()) {
			vt.push_back(this->doubleSpinBox4->value());
		}

        object.value = ValuePtr(vt);
	}
	emit changed();
}

ValuePtr ParameterEntryWidget::getValue()
{
    return object.value;
}

bool ParameterEntryWidget::isDefaultValue()
{
    return object.value == object.defaultValue;
}

void ParameterEntryWidget::applyParameter(Assignment *assignment)
{
    object.applyParameter(assignment);
}

void ParameterEntryWidget::setAssignment(Context *ctx, const Assignment *assignment, const ValuePtr defaultValue)
{
	const std::string name = assignment->first;

	const Annotation *param = assignment->annotation("Parameter");
	const ValuePtr values = param->evaluate(ctx, "values");

	setName(QString::fromStdString(name));
	setValue(defaultValue, values);

	const Annotation *desc = assignment->annotation("Description");
	if (desc) {
		const ValuePtr v = desc->evaluate(ctx, "text");
		if (v->type() == Value::STRING) {
			setDescription(QString::fromStdString(v->toString()));
		}
	}
}

void ParameterEntryWidget::setName(const QString& name)
{
	this->labelDescription->hide();
	this->labelParameter->setText(name);
}

void ParameterEntryWidget::setValue(const ValuePtr defaultValue, const ValuePtr values)
{

    object.setValue(defaultValue,values);
    switch (object.target) {
    case 1:
	{
		this->stackedWidget->setCurrentWidget(this->pageComboBox);
		comboBox->clear();
        const Value::VectorType& vec = object.values->toVector();
		for (Value::VectorType::const_iterator it = vec.begin(); it != vec.end(); it++) {
			const ValuePtr *v = &(*it);
			comboBox->addItem(QString::fromStdString((*it)->toString()), qVariantFromValue((void *)v));
		}
        QString defaultText = QString::fromStdString(object.value->toString());
		int idx = comboBox->findText(defaultText);
		if (idx >= 0) {
			comboBox->setCurrentIndex(idx);
		}
		break;
	}
    case 2:
    {
        this->stackedWidget->setCurrentWidget(this->pageSlider);
        this->slider->setMaximum(object.values->toRange().end_value());
        this->slider->setValue(object.value->toDouble());
        this->slider->setMinimum(object.values->toRange().begin_value());
        break;
    }
    case 3:
	{
		this->stackedWidget->setCurrentWidget(this->pageCheckBox);
        this->checkBox->setChecked(object.value->toBool());
		break;
	}
    case 4:{
        this->stackedWidget->setCurrentWidget(this->pageText);
        this->lineEdit->setText(QString::fromStdString(object.defaultValue->toString()));
        break;
    }
    case 5:
	{
		this->stackedWidget->setCurrentWidget(this->pageVector);
        this->doubleSpinBox1->setValue(object.value->toDouble());
		this->doubleSpinBox2->hide();
		this->doubleSpinBox3->hide();
		this->doubleSpinBox4->hide();
		break;
	}
    case 6:
	{
		this->stackedWidget->setCurrentWidget(this->pageVector);
        Value::VectorType vec = object.defaultValue->toVector();
		if (vec.size() < 4) {
			this->doubleSpinBox4->hide();
			this->doubleSpinBox4->setReadOnly(true);
		}
		if (vec.size() < 3) {
			this->doubleSpinBox3->hide();
			this->doubleSpinBox3->setReadOnly(true);
		}
		if (vec.size() < 2) {
			this->doubleSpinBox2->hide();
			this->doubleSpinBox2->setReadOnly(true);
		}
		this->doubleSpinBox1->setValue(vec.at(0)->toDouble());
		if (vec.size() > 1) {
			this->doubleSpinBox2->setValue(vec.at(1)->toDouble());
			this->doubleSpinBox2->setReadOnly(false);
		}
		if (vec.size() > 2) {
			this->doubleSpinBox3->setValue(vec.at(2)->toDouble());
			this->doubleSpinBox3->setReadOnly(false);
		}
		if (vec.size() > 3) {
			this->doubleSpinBox4->setValue(vec.at(3)->toDouble());
			this->doubleSpinBox4->setReadOnly(false);
		}
	}
	}
}

void ParameterEntryWidget::setDescription(const QString& description)
{
	this->labelDescription->show();
	this->labelDescription->setText(description);
}

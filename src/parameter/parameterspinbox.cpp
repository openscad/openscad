#include "parameterspinbox.h"

ParameterSpinBox::ParameterSpinBox(ParameterObject *parameterobject, bool showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(doubleSpinBox1, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
	if (showDescription == true) {
		setDescription(object->description);
	}
	else {
		doubleSpinBox1->setToolTip(object->description);
	}
}

void ParameterSpinBox::onChanged(double)
{
	object->focus = true;
	object->value = ValuePtr(doubleSpinBox1->value());
	emit changed();
}

void ParameterSpinBox::setParameterFocus()
{
	this->doubleSpinBox1->setFocus();
	object->focus = false;
}

void ParameterSpinBox::setValue()
{
	if (object->values->toDouble() > 0) {
		setPrecision(object->values->toDouble());
		this->doubleSpinBox1->setSingleStep(object->values->toDouble());
	}
	else {
		setPrecision(object->defaultValue->toDouble());
		this->doubleSpinBox1->setSingleStep(1/pow(10,decimalPrecision));
	}
	this->doubleSpinBox1->setDecimals(decimalPrecision);
	this->stackedWidget->setCurrentWidget(this->pageVector);
	this->doubleSpinBox1->setRange(object->value->toDouble()-1000, object->value->toDouble()+1000);
	this->doubleSpinBox1->setValue(object->value->toDouble());
	
	this->doubleSpinBox2->hide();
	this->doubleSpinBox3->hide();
	this->doubleSpinBox4->hide();
}

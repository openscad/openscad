#include "parametervector.h"

ParameterVector::ParameterVector(ParameterObject *parameterobject, bool showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(doubleSpinBox1,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox2,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox3,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox4,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	if (showDescription == true) {
		setDescription(object->description);
	}
	else {
		frame->setToolTip(object->description);
	}
}

void ParameterVector::onChanged(double)
{
	object->focus = true;
	if (object->target == 5) {
		object->value = ValuePtr(doubleSpinBox1->value());
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
		object->value = ValuePtr(vt);
	}
	emit changed();
}

void ParameterVector::setParameterFocus()
{
	this->doubleSpinBox1->setFocus();
	object->focus = false;
}

void ParameterVector::setValue()
{
	this->stackedWidget->setCurrentWidget(this->pageVector);
	Value::VectorType vec = object->value->toVector();
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
	setPrecision(vec.at(0)->toDouble());
	this->doubleSpinBox1->setDecimals(decimalPrecision);
	this->doubleSpinBox1->setRange(vec.at(0)->toDouble()-1000,vec.at(0)->toDouble()+1000);
	this->doubleSpinBox1->setValue(vec.at(0)->toDouble());
	if (vec.size() > 1) {
		setPrecision(vec.at(1)->toDouble());
		this->doubleSpinBox2->setDecimals(decimalPrecision);
		this->doubleSpinBox2->setRange(vec.at(1)->toDouble()-1000,vec.at(1)->toDouble()+1000);
		this->doubleSpinBox2->setValue(vec.at(1)->toDouble());
		this->doubleSpinBox2->setReadOnly(false);
	}
	if (vec.size() > 2) {
		setPrecision(vec.at(2)->toDouble());
		this->doubleSpinBox3->setDecimals(decimalPrecision);
		this->doubleSpinBox3->setRange(vec.at(2)->toDouble()-1000,vec.at(2)->toDouble()+1000);
		this->doubleSpinBox3->setValue(vec.at(2)->toDouble());
		this->doubleSpinBox3->setReadOnly(false);
	}
	if (vec.size() > 3) {
		setPrecision(vec.at(3)->toDouble());
		this->doubleSpinBox4->setDecimals(decimalPrecision);
		this->doubleSpinBox4->setRange(vec.at(3)->toDouble()-1000,vec.at(3)->toDouble()+1000);
		this->doubleSpinBox4->setValue(vec.at(3)->toDouble());
		this->doubleSpinBox4->setReadOnly(false);
	}
}

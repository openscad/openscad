#include "parameterslider.h"

ParameterSlider::ParameterSlider(ParameterObject *parameterobject, bool showDescription)
{
	this->pressed = true;
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(slider, SIGNAL(sliderPressed()), this, SLOT(onPressed()));
	connect(slider, SIGNAL(sliderReleased()), this, SLOT(onReleased()));
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
	if (showDescription == true) {
		setDescription(object->description);
	}
	else {
		slider->setToolTip(object->description);
	}
}

void ParameterSlider::onChanged(int)
{
	double v = slider->value()*step;
	this->labelSliderValue->setText(QString::number(v, 'f', decimalPrecision));
	if (this->pressed) {
		object->focus = true;
		object->value = ValuePtr(v);
		emit changed();
	}
}

void ParameterSlider::setParameterFocus()
{
	slider->setFocus();
	object->focus = false;
}

void ParameterSlider::onPressed()
{
	this->pressed = false;
}

void ParameterSlider::onReleased(){
	this->pressed = true;
	onChanged(0);
}

void ParameterSlider::setValue()
{
	if (object->values->toRange().step_value() > 0) {
		setPrecision(object->values->toRange().step_value());
		step = object->values->toRange().step_value();
	} else {
		decimalPrecision = 1;
		step = 1;
	}
	int min = object->values->toRange().begin_value()/step;
	int max=object->values->toRange().end_value()/step;
	int current=object->value->toDouble()/step;
	this->stackedWidget->setCurrentWidget(this->pageSlider);
	this->slider->setRange(min,max);
	this->slider->setValue(current);
	this->labelSliderValue->setText(QString::number(current*step, 'f',decimalPrecision));
}

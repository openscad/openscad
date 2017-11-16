#include "parameterslider.h"

ParameterSlider::ParameterSlider(ParameterObject *parameterobject, int showDescription)
{
	this->pressed = true;
	this->suppressUpdate=false;

	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(slider, SIGNAL(sliderPressed()), this, SLOT(onPressed()));
	connect(slider, SIGNAL(sliderReleased()), this, SLOT(onReleased()));
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
	if (showDescription == 0) {
		setDescription(object->description);
	}else if(showDescription == 1){
		addInline(object->description);
	}else {
		slider->setToolTip(object->description);
	}
}

void ParameterSlider::onSliderChanged(int)
{
	double v = slider->value()*step;

	if (!this->suppressUpdate) {
		this->doubleSpinBox->setValue(v);
	}

	if (this->pressed) {
		object->focus = true;
		object->value = ValuePtr(v);
		emit changed();
	}
}

void ParameterSlider::onSpinBoxChanged(double v)
{
	if (!this->suppressUpdate) {
		if(v>0){
			this->slider->setValue((int)((v+step/2.0)/step));
		}else{
			this->slider->setValue((int)((v-step/2.0)/step));
		}
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
	onSliderChanged(0);
}

void ParameterSlider::setValue()
{
	if(hasFocus())return; //refuse programmatic updates, when the widget is in the focus of the user

	suppressUpdate=true;
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
	this->stackedWidgetBelow->setCurrentWidget(this->pageSlider);
	this->pageSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	this->slider->setRange(min,max);
	this->slider->setValue(current);

	this->stackedWidgetRight->setCurrentWidget(this->pageSpin);
	this->pageSpin->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->doubleSpinBox->setMinimum(object->values->toRange().begin_value());
	this->doubleSpinBox->setMaximum(object->values->toRange().end_value());
	this->doubleSpinBox->setSingleStep(object->values->toRange().step_value());
	this->doubleSpinBox->setDecimals(decimalPrecision);
	this->doubleSpinBox->setValue(object->value->toDouble());
	suppressUpdate=false;
}

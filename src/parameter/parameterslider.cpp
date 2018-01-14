#include "parameterslider.h"
#include "ignoreWheelWhenNotFocused.h"

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

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused();
	slider->installEventFilter(ignoreWheelWhenNotFocused);
	doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);
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
	int minSlider = 0;
	int maxSlider = 0;
	double min=0;
	double max=0;
	if(object->values->type() == Value::ValueType::RANGE ){ // [min:max] and [min:step:max] format
		minSlider = object->values->toRange().begin_value()/step;
		maxSlider = object->values->toRange().end_value()/step;
		
		min = object->values->toRange().begin_value();
		max = object->values->toRange().end_value();
	}else{ // [max] format from makerbot customizer
		step = 1;
		maxSlider =  std::stoi(object->values->toVector()[0]->toString(),nullptr,0);
		max = maxSlider;
		setPrecision(1);
	}

	int current=object->value->toDouble()/step;
	this->stackedWidgetBelow->setCurrentWidget(this->pageSlider);
	this->pageSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	this->slider->setRange(minSlider,maxSlider);
	this->slider->setValue(current);

	this->stackedWidgetRight->setCurrentWidget(this->pageSpin);
	this->pageSpin->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->doubleSpinBox->setMinimum(min);
	this->doubleSpinBox->setMaximum(max);
	this->doubleSpinBox->setSingleStep(step);
	this->doubleSpinBox->setDecimals(decimalPrecision);
	this->doubleSpinBox->setValue(object->value->toDouble());
	suppressUpdate=false;
}

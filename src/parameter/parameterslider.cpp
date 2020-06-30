#include "parameterslider.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterSlider::ParameterSlider(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD), suppressUpdate(false)
{
	setValue();

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));

	connect(slider, SIGNAL(sliderReleased()), this, SLOT(onEditingFinished()));
	connect(doubleSpinBox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	slider->installEventFilter(ignoreWheelWhenNotFocused);
	doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterSlider::onSliderChanged(int)
{
	if (!this->suppressUpdate) {
		const double v = doubleSpinBox->minimum() + slider->value() * step;
		PRINTDB("onSliderChanged(): %.2f (slider = %d)", v % slider->value());
		this->suppressUpdate = true;
		this->doubleSpinBox->setValue(v);
		this->suppressUpdate = false;
	}
}

void ParameterSlider::onSpinBoxChanged(double v)
{
	if (!this->suppressUpdate) {
		PRINTDB("onSpinBoxChanged(): %.2f", v);
		this->suppressUpdate = true;
		slider->setValue(std::nextafter((v - doubleSpinBox->minimum()) / step, max_uint32));
		this->suppressUpdate=false;
	}
}

void ParameterSlider::onEditingFinished()
{
	const double v = doubleSpinBox->value();
	PRINTDB("updateValue(): %.2f", v);
	object->value = Value(v);
	emit changed();
}

void ParameterSlider::setValue()
{
	this->suppressUpdate = true;
	const double v = object->value.toDouble();

	if (object->values.toRange().step_value() > 0) {
		setPrecision(object->values.toRange().step_value());
		step = object->values.toRange().step_value();
	} else {
		decimalPrecision = 1;
		step = 1;
	}

	double min = 0;
	double max = 0;
	int maxSlider = 0;
	int curSlider = 0;
	if (object->values.type() == Value::Type::RANGE) {
		// [min:max] and [min:step:max] format
		const double b = object->values.toRange().begin_value();
		const double e = object->values.toRange().end_value();
		maxSlider = std::nextafter((e - b) / step, max_uint32);
		curSlider = std::nextafter((v - b) / step, max_uint32);
		min = b;
		// Truncate end value to full steps, same as Thingiverse customizer.
		// This also makes sure the step size of the spin box does not go to
		// invalid values.
		max = b + maxSlider * step;
	} else {
		// [max] format from makerbot customizer
		decimalPrecision = 0;
		step = 1;
		maxSlider =  std::stoi(object->values.toVector()[0].toString());
		curSlider = v;
		max = maxSlider;
	}

	this->stackedWidgetBelow->setCurrentWidget(this->pageSlider);
	this->pageSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->slider->setRange(0, maxSlider);
	this->slider->setValue(curSlider);

	this->stackedWidgetRight->setCurrentWidget(this->pageSpin);
	this->pageSpin->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->doubleSpinBox->setMinimum(min);
	this->doubleSpinBox->setMaximum(max);
	this->doubleSpinBox->setSingleStep(step);
	this->doubleSpinBox->setDecimals(decimalPrecision);
	this->doubleSpinBox->setValue(v);
	this->suppressUpdate = false;
	PRINTDB("ParameterSlider: [%.2f:%.2f:%.2f, %.2f] - (%d - %d, %d)", min % step % max % v % 0 % maxSlider % curSlider);
}

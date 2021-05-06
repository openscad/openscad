#include "parameterslider.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterSlider::ParameterSlider(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle):
	ParameterVirtualWidget(parent, parameter),
	parameter(parameter)
{
	setupUi(this);
	descriptionWidget->setDescription(parameter, descriptionStyle);

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	slider->installEventFilter(ignoreWheelWhenNotFocused);
	doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);

	assert(parameter->minimum);
	assert(parameter->maximum);

	int decimals;
	this->minimum = *parameter->minimum;
	if (parameter->step) {
		this->step = *parameter->step;
		decimals = decimalsRequired({
			this->minimum,
			parameter->defaultValue,
			this->step
		});
	} else {
		decimals = decimalsRequired({
			this->minimum,
			parameter->defaultValue
		});
		this->step = pow(0.1, decimals);
	}
	// Truncate end value to full steps, same as Thingiverse customizer.
	// This also makes sure the step size of the spin box does not go to
	// invalid values.
	int maximumSliderPosition = sliderPosition(*parameter->maximum);
	double maximumValue = parameterValue(maximumSliderPosition);

	slider->setRange(0, maximumSliderPosition);
	doubleSpinBox->setDecimals(decimals);
	doubleSpinBox->setRange(this->minimum, maximumValue);
	doubleSpinBox->setSingleStep(this->step);

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));

	connect(slider, SIGNAL(sliderReleased()), this, SLOT(onEditingFinished()));
	connect(doubleSpinBox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));

	setValue();
}

void ParameterSlider::onSliderChanged(int position)
{
	if (!inUpdate) {
		inUpdate = true;
		double value = parameterValue(position);
		doubleSpinBox->setValue(value);
		inUpdate = false;
	}
}

void ParameterSlider::onSpinBoxChanged(double value)
{
	if (!inUpdate) {
		inUpdate = true;
		int position = sliderPosition(value);
		slider->setValue(position);
		inUpdate = false;
	}
}

void ParameterSlider::onEditingFinished()
{
	parameter->value = parameterValue(slider->value());
	emit changed();
}

void ParameterSlider::setValue()
{
	inUpdate = true;
	int position = sliderPosition(parameter->value);
	slider->setValue(position);
	doubleSpinBox->setValue(parameterValue(position));
	inUpdate = false;
}

int ParameterSlider::sliderPosition(double value)
{
	return std::nextafter((value - this->minimum) / this->step, std::numeric_limits<uint32_t>::max());
}

double ParameterSlider::parameterValue(int sliderPosition)
{
	return this->minimum + sliderPosition * this->step;
}

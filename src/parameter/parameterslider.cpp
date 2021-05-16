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
	slider->setStyle(new SliderStyleJumpTo(slider->style()));
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

	static constexpr double maxSteps = static_cast<double>(std::numeric_limits<int>::max());
	// Use nextafter to compensate for possible floating point inaccurary where result is just below a whole number.
	double tempSteps = std::nextafter((*parameter->maximum - this->minimum) / this->step, maxSteps) + 1.0;
	int numSteps =  tempSteps >= maxSteps ? std::numeric_limits<int>::max() : static_cast<int>(tempSteps);
	// Truncate end value to full steps, same as Thingiverse customizer.
	// This also makes sure the step size of the spin box does not go to
	// invalid values.
	double maximumValue = parameterValue(numSteps-1);

	slider->setRange(0, numSteps-1);
	doubleSpinBox->setKeyboardTracking(false);
	doubleSpinBox->setDecimals(decimals);
	doubleSpinBox->setRange(this->minimum, maximumValue);
	doubleSpinBox->setSingleStep(this->step);

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
	connect(slider, SIGNAL(sliderReleased()), this, SLOT(onEditingFinished()));

	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
	connect(doubleSpinBox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));

	setValue();
}

void ParameterSlider::onSliderChanged(int position)
{
	double value = parameterValue(position);
	doubleSpinBox->setValue(value);
}

void ParameterSlider::onSpinBoxChanged(double value)
{
	int position = sliderPosition(value);
	slider->setSliderPosition(position);
}

void ParameterSlider::onEditingFinished()
{
	double val = parameterValue(slider->sliderPosition());
	if (val != parameter->value) {
		parameter->value = val;
		emit changed();
	}
}

void ParameterSlider::setValue()
{
	int position = sliderPosition(parameter->value);
	slider->setSliderPosition(position);
	doubleSpinBox->setValue(parameterValue(position));
}

int ParameterSlider::sliderPosition(double value)
{
	return static_cast<int>(std::round((value - this->minimum) / this->step));
}

double ParameterSlider::parameterValue(int sliderPosition)
{
	return this->minimum + sliderPosition * this->step;
}

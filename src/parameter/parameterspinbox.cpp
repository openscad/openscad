#include "parameterspinbox.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterSpinBox::ParameterSpinBox(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle):
	ParameterVirtualWidget(parent, parameter),
	parameter(parameter)
{
	setupUi(this);
	descriptionWidget->setDescription(parameter, descriptionStyle);

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);

	int decimals = decimalsRequired(parameter->defaultValue);
	double minimum;
	if (parameter->minimum) {
		minimum = *parameter->minimum;
		decimals = std::max(decimals, decimalsRequired(minimum));
	} else if (parameter->maximum && *parameter->maximum > 0) {
		minimum = 0;
	} else {
		minimum = std::numeric_limits<double>::lowest();
	}
	double maximum;
	if (parameter->maximum) {
		maximum = *parameter->maximum;
		decimals = std::max(decimals, decimalsRequired(maximum));
	} else if (parameter->minimum && *parameter->minimum < 0) {
		maximum = 0;
	} else {
		maximum = std::numeric_limits<double>::max();
	}
	double step;
	if (parameter->step) {
		step = *parameter->step;
		decimals = std::max(decimals, decimalsRequired(step));
	} else {
		step = pow(0.1, decimals);
	}
	doubleSpinBox->setDecimals(decimals);
	doubleSpinBox->setRange(minimum, maximum);
	doubleSpinBox->setSingleStep(step);

	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChanged()));
	setValue();
}

void ParameterSpinBox::onChanged()
{
	if (!inUpdate) {
		parameter->value = doubleSpinBox->value();
		emit changed();
	}
}

void ParameterSpinBox::setValue()
{
	inUpdate = true;
	doubleSpinBox->setValue(parameter->value);
	inUpdate = false;
}

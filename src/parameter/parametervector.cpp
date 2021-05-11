#include "parametervector.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterVector::ParameterVector(QWidget *parent, VectorParameter *parameter, DescriptionStyle descriptionStyle):
	ParameterVirtualWidget(parent, parameter),
	parameter(parameter)
{
	setupUi(this);
	descriptionWidget->setDescription(parameter, descriptionStyle);

	assert(parameter->defaultValue.size() >= 1);
	assert(parameter->defaultValue.size() <= 4);

	if (parameter->defaultValue.size() >= 1) {
		spinboxes.push_back(doubleSpinBox1);
	} else {
		doubleSpinBox1->hide();
	}
	if (parameter->defaultValue.size() >= 2) {
		spinboxes.push_back(doubleSpinBox2);
	} else {
		doubleSpinBox2->hide();
	}
	if (parameter->defaultValue.size() >= 3) {
		spinboxes.push_back(doubleSpinBox3);
	} else {
		doubleSpinBox3->hide();
	}
	if (parameter->defaultValue.size() >= 4) {
		spinboxes.push_back(doubleSpinBox4);
	} else {
		doubleSpinBox4->hide();
	}

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	for (auto spinbox : spinboxes) {
		spinbox->installEventFilter(ignoreWheelWhenNotFocused);
	}

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
	for (auto spinbox : spinboxes) {
		spinbox->setDecimals(decimals);
		spinbox->setRange(minimum, maximum);
		spinbox->setSingleStep(step);
		spinbox->show();
		connect(spinbox, SIGNAL(valueChanged(double)), this, SLOT(onChanged()));
	}

	setValue();
}

void ParameterVector::onChanged()
{
	if (!inUpdate) {
		for (size_t i = 0; i < spinboxes.size(); i++) {
			parameter->value[i] = spinboxes[i]->value();
		}
		emit changed();
	}
}

void ParameterVector::setValue()
{
	inUpdate = true;
	for (size_t i = 0; i < spinboxes.size(); i++) {
		spinboxes[i]->setValue(parameter->value[i]);
	}
	inUpdate = false;
}

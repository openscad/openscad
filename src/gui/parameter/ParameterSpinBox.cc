#include "gui/parameter/ParameterSpinBox.h"
#include <QWidget>
#include <algorithm>
#include <limits>
#include "gui/IgnoreWheelWhenNotFocused.h"

ParameterSpinBox::ParameterSpinBox(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle) :
  ParameterVirtualWidget(parent, parameter),
  parameter(parameter)
{
  setupUi(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);

  auto *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
  doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);
  doubleSpinBox->setKeyboardTracking(true);

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

  connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
  connect(doubleSpinBox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  ParameterSpinBox::setValue();
}

void ParameterSpinBox::valueApplied() {
  lastApplied = lastSent;
}

void ParameterSpinBox::onChanged(double value)
{
#ifdef DEBUG
  PRINTD(STR("[changed] value=", value, ", parameter->value=", parameter->value, ", lastSent=", lastSent, ", lastApplied=", lastApplied));
#endif
  parameter->value = value;
  if (lastSent != value) {
    lastSent = value;
    emit changed(false);
  }
}

void ParameterSpinBox::onEditingFinished()
{
#ifdef DEBUG
  PRINTD(STR("[finished] parameter->value=", parameter->value, ", lastSent=", lastSent, ", lastApplied=", lastApplied));
#endif
  if (lastApplied != parameter->value) {
    lastSent = parameter->value;
    emit changed(true);
  }
}

void ParameterSpinBox::setValue()
{
  lastApplied = lastSent = parameter->value;
  doubleSpinBox->setValue(parameter->value);
}

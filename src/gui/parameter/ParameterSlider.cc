#include "gui/parameter/ParameterSlider.h"
#include <QWidget>
#include <cmath>
#include <cassert>
#include <limits>
#include "gui/IgnoreWheelWhenNotFocused.h"

ParameterSlider::ParameterSlider(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle) :
  ParameterVirtualWidget(parent, parameter),
  parameter(parameter)
{
  setupUi(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);

  auto *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
  slider->installEventFilter(ignoreWheelWhenNotFocused);
  doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);
  doubleSpinBox->setKeyboardTracking(true);

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

  static constexpr auto maxSteps = static_cast<double>(std::numeric_limits<int>::max());
  // Use nextafter to compensate for possible floating point inaccurary where result is just below a whole number.
  double tempSteps = std::nextafter((*parameter->maximum - this->minimum) / this->step, maxSteps) + 1.0;
  int numSteps = tempSteps >= maxSteps ? std::numeric_limits<int>::max() : static_cast<int>(tempSteps);
  // Truncate end value to full steps, same as Thingiverse customizer.
  // This also makes sure the step size of the spin box does not go to
  // invalid values.
  double maximumValue = parameterValue(numSteps - 1);

  slider->setRange(0, numSteps - 1);
  slider->setPageStep(std::ceil(0.1 * numSteps));
  doubleSpinBox->setDecimals(decimals);
  doubleSpinBox->setRange(this->minimum, maximumValue);
  doubleSpinBox->setSingleStep(this->step);

  //connect(slider, SIGNAL(sliderPressed()), this, SLOT(onSliderPressed()));
  connect(slider, SIGNAL(sliderReleased()), this, SLOT(onSliderReleased()));
  connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMoved(int)));
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));

  connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged(double)));
  connect(doubleSpinBox, SIGNAL(editingFinished()), this, SLOT(onSpinBoxEditingFinished()));

  ParameterSlider::setValue();
}

void ParameterSlider::valueApplied() {
  lastApplied = lastSent;
}

// slider handle grabbed
void ParameterSlider::onSliderPressed()
{
}

// slider handle released
void ParameterSlider::onSliderReleased()
{
  this->commitChange(true);
}

// slider handle dragged
void ParameterSlider::onSliderMoved(int position)
{
  double value = parameterValue(position);
  doubleSpinBox->blockSignals(true);
  doubleSpinBox->setValue(value);
  doubleSpinBox->blockSignals(false);
}

// slider track clicked
// changes by pageStep or sets absolute position, depending on platform or specific mouse button
void ParameterSlider::onSliderChanged(int position)
{
  double value = parameterValue(position);
  doubleSpinBox->blockSignals(true);
  doubleSpinBox->setValue(value);
  doubleSpinBox->blockSignals(false);
  commitChange(false);
}

// spin button click or arrow keypress
void ParameterSlider::onSpinBoxChanged(double value)
{
  int position = sliderPosition(value);
  slider->blockSignals(true);
  slider->setValue(position);
  slider->blockSignals(false);
  commitChange(false);
}

// Enter key pressed or spinbox focus lost
void ParameterSlider::onSpinBoxEditingFinished()
{
  commitChange(true);
}

void ParameterSlider::commitChange(bool immediate) {
  double value = parameterValue(slider->sliderPosition());
#ifdef DEBUG
  PRINTD(STR("[commit] value=", value, ", parameter->value=", parameter->value, ", lastSent=", lastSent, ", lastApplied=", lastApplied));
#endif
  if ((immediate && lastApplied != value) || (!immediate && lastSent != value) ) {
    lastSent = parameter->value = value;
    emit changed(immediate);
  }
}

// Called when populating parameter presets
void ParameterSlider::setValue()
{
#ifdef DEBUG
  PRINTD(STR("[setValue] parameter->value=", parameter->value, ", lastSent=", lastSent, ", lastApplied=", lastApplied));
#endif
  int position = sliderPosition(parameter->value);
  lastApplied = lastSent = parameter->value;
  slider->setValue(position);
}

int ParameterSlider::sliderPosition(double value)
{
  return static_cast<int>(std::round((value - this->minimum) / this->step));
}

double ParameterSlider::parameterValue(int sliderPosition)
{
  return this->minimum + sliderPosition * this->step;
}

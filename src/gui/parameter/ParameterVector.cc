#include "gui/parameter/ParameterVector.h"

#include <QWidget>
#include <algorithm>
#include <cassert>
#include <limits>
#include <cstddef>
#include "gui/IgnoreWheelWhenNotFocused.h"

ParameterVector::ParameterVector(QWidget *parent, VectorParameter *parameter, DescriptionStyle descriptionStyle) :
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

  // clang generates a bogus warning that ignoreWheelWhenNotFocused may be leaked
  // NOLINTBEGIN(*NewDeleteLeaks)
  if (spinboxes.size() > 0) { // only allocate if there are spinboxes to use the function
    // The parent (this) takes ownership of the object
    auto *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
    for (auto spinbox : spinboxes) {
      spinbox->installEventFilter(ignoreWheelWhenNotFocused);
    }
  }

  int decimals = decimalsRequired(parameter->defaultValue);
  double minimum;
  // NOLINTEND(*NewDeleteLeaks)
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
    connect(spinbox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  }

  ParameterVector::setValue();
}

void ParameterVector::valueApplied() {
  lastApplied = lastSent;
}

void ParameterVector::onChanged()
{
  for (size_t i = 0; i < spinboxes.size(); i++) {
    parameter->value[i] = spinboxes[i]->value();
  }
  if (parameter->value != lastSent) {
    lastSent = parameter->value;
    emit changed(false);
  }
}

void ParameterVector::onEditingFinished()
{
  if (lastApplied != parameter->value) {
    lastSent = parameter->value;
    emit changed(true);
  }
}

void ParameterVector::setValue()
{
#ifdef DEBUG
  PRINTD("setValue");
#endif
  lastApplied = lastSent = parameter->value;
  for (size_t i = 0; i < spinboxes.size(); i++) {
    // don't emit valueChanged signal for initial setup
    spinboxes[i]->blockSignals(true);
    spinboxes[i]->setValue(parameter->value[i]);
    spinboxes[i]->blockSignals(false);
  }
}

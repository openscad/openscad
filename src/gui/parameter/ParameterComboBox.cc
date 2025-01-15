#include "gui/parameter/ParameterComboBox.h"
#include <QString>
#include <QWidget>
#include "gui/IgnoreWheelWhenNotFocused.h"

ParameterComboBox::ParameterComboBox(QWidget *parent, EnumParameter *parameter, DescriptionStyle descriptionStyle) :
  ParameterVirtualWidget(parent, parameter),
  parameter(parameter)
{
  setupUi(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);

  auto *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
  comboBox->installEventFilter(ignoreWheelWhenNotFocused);

  for (const auto& item : parameter->items) {
    comboBox->addItem(QString::fromStdString(item.key));
  }
  #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  connect(comboBox, &QComboBox::activated, this, &ParameterComboBox::onChanged);
  #else
  connect(comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &ParameterComboBox::onChanged);
  #endif

  ParameterComboBox::setValue();
}

void ParameterComboBox::onChanged(int index)
{
  if (parameter->valueIndex != index) {
    parameter->valueIndex = index;
    emit changed(true);
  }
}

void ParameterComboBox::setValue()
{
  comboBox->setCurrentIndex(parameter->valueIndex);
}

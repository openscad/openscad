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

  connect(comboBox, SIGNAL(activated(int)), this, SLOT(onChanged(int)));
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

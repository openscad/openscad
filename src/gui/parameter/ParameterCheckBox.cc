#include <QWidget>
#include "gui/parameter/ParameterCheckBox.h"

ParameterCheckBox::ParameterCheckBox(QWidget *parent, BoolParameter *parameter, DescriptionStyle descriptionStyle) :
  ParameterVirtualWidget(parent, parameter),
  parameter(parameter)
{
  setupUi(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);

  if (descriptionStyle == DescriptionStyle::ShowDetails) {
    //large checkbox, when we have the space
    checkBox->setStyleSheet("QCheckBox::indicator { width: 20px; height: 20px; } QCheckBox { spacing: 0px; }");
  }

  connect(checkBox, SIGNAL(clicked()), this, SLOT(onChanged()));
  ParameterCheckBox::setValue();
}

void ParameterCheckBox::onChanged()
{
  parameter->value = checkBox->isChecked();
  emit changed(true);
}

void ParameterCheckBox::setValue()
{
  checkBox->setChecked(parameter->value);
}

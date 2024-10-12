#pragma once

#include "gui/parameter/ParameterVirtualWidget.h"
#include <QWidget>
#include "ui_ParameterCheckBox.h"

class ParameterCheckBox : public ParameterVirtualWidget, Ui::ParameterCheckBox
{
  Q_OBJECT

public:
  ParameterCheckBox(QWidget *parent, BoolParameter *parameter, DescriptionStyle descriptionStyle);
  void setValue() override;

protected slots:
  void onChanged();

private:
  BoolParameter *parameter;
};

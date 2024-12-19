#pragma once

#include "gui/parameter/ParameterVirtualWidget.h"
#include <QWidget>
#include "ui_ParameterSpinBox.h"

class ParameterSpinBox : public ParameterVirtualWidget, Ui::ParameterSpinBox
{
  Q_OBJECT

public:
  ParameterSpinBox(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle);
  void setValue() override;
  void valueApplied() override;

protected slots:
  void onChanged(double);
  void onEditingFinished();

private:
  NumberParameter *parameter;
  boost::optional<double> lastSent;
  boost::optional<double> lastApplied;

};

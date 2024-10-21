#pragma once

#include "gui/parameter/ParameterVirtualWidget.h"
#include "ui_ParameterVector.h"

#include <QDoubleSpinBox>
#include <QWidget>
#include <vector>

class ParameterVector : public ParameterVirtualWidget, Ui::ParameterVector
{
  Q_OBJECT

public:
  ParameterVector(QWidget *parent, VectorParameter *parameter, DescriptionStyle descriptionStyle);
  void setValue() override;
  void valueApplied() override;

protected slots:
  void onChanged();
  void onEditingFinished();

private:
  VectorParameter *parameter;
  std::vector<QDoubleSpinBox *> spinboxes;
  std::vector<double> lastSent;
  std::vector<double> lastApplied;
};

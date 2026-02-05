#pragma once

#include "gui/parameter/ParameterVirtualWidget.h"
#include <QPushButton>

class ColorParameter;

class ParameterColor : public ParameterVirtualWidget
{
  Q_OBJECT

public:
  ParameterColor(QWidget *parent, ColorParameter *parameter, DescriptionStyle descriptionStyle);
  void setValue() override;

protected slots:
  void onClicked();

private:
  ColorParameter *parameter;
  QPushButton *colorButton;
  ParameterDescriptionWidget *descriptionWidget;

  void updateButtonColor();
};

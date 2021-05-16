#pragma once

#include <QProxyStyle>
#include "parametervirtualwidget.h"
#include "ui_parameterslider.h"

class SliderStyleJumpTo : public QProxyStyle
{
public:
  using QProxyStyle::QProxyStyle;

  int styleHint(QStyle::StyleHint hint, const QStyleOption* option = 0, const QWidget* widget = 0, QStyleHintReturn* returnData = 0) const
  {
    if (hint == QStyle::SH_Slider_AbsoluteSetButtons) {
      return Qt::LeftButton;
    } else if (hint == QStyle::SH_Slider_PageSetButtons) {
      return Qt::NoButton;
    }
    return QProxyStyle::styleHint(hint, option, widget, returnData);
  }
};

class ParameterSlider : public ParameterVirtualWidget, Ui::ParameterSlider
{
	Q_OBJECT

public:
	ParameterSlider(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onSpinBoxChanged(double);
	void onSliderChanged(int);
	void onEditingFinished();

private:
	NumberParameter* parameter;
	double minimum;
	double step;
	
	int sliderPosition(double value);
	double parameterValue(int sliderPosition);
};

#pragma once

#include "parametervirtualwidget.h"
#include "ui_parameterslider.h"

class ParameterSlider : public ParameterVirtualWidget, Ui::ParameterSlider
{
	Q_OBJECT

public:
	ParameterSlider(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onSliderChanged(int);
	void onSpinBoxChanged(double);
	void onEditingFinished();

private:
	NumberParameter* parameter;
	double minimum;
	double step;
	bool inUpdate = false;
	
	int sliderPosition(double value);
	double parameterValue(int sliderPosition);
};

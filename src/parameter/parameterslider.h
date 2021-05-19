#pragma once

#include "parametervirtualwidget.h"
#include "ui_parameterslider.h"

class ParameterSlider : public ParameterVirtualWidget, Ui::ParameterSlider
{
	Q_OBJECT

public:
	ParameterSlider(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;
	void valueApplied() override;

protected slots:
	void onSliderPressed();
	void onSliderReleased();
	void onSliderMoved(int position);
	void onSliderChanged(int position);

	void onSpinBoxChanged(double value);
	void onSpinBoxEditingFinished();

private:
	NumberParameter* parameter;
	boost::optional<double> lastSent;
	boost::optional<double> lastApplied;
	double minimum;
	double step;

	int sliderPosition(double value);
	double parameterValue(int sliderPosition);
	void commitChange(bool immediate);
};

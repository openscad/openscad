#pragma once

#include "parametervirtualwidget.h"
#include "ui_parameterspinbox.h"

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
	NumberParameter* parameter;
	boost::optional<double> lastSent;
	boost::optional<double> lastApplied;

};

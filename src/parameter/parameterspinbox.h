#pragma once

#include "parametervirtualwidget.h"
#include "ui_parameterspinbox.h"

class ParameterSpinBox : public ParameterVirtualWidget, Ui::ParameterSpinBox
{
	Q_OBJECT

public:
	ParameterSpinBox(QWidget *parent, NumberParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onChanged();

private:
	NumberParameter* parameter;
	bool inUpdate = false;
};

#pragma once

#include "parametervirtualwidget.h"
#include "ui_parametertext.h"

class ParameterText : public ParameterVirtualWidget, Ui::ParameterText
{
	Q_OBJECT

public:
	ParameterText(QWidget *parent, StringParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onChanged();

private:
	StringParameter* parameter;
};

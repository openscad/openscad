#pragma once

#include "parametervirtualwidget.h"
#include "ui_parametercheckbox.h"

class ParameterCheckBox : public ParameterVirtualWidget, Ui::ParameterCheckBox
{
	Q_OBJECT

public:
	ParameterCheckBox(QWidget *parent, BoolParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onChanged();

private:
	BoolParameter* parameter;
};

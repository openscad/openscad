#pragma once

#include "parametervirtualwidget.h"
#include "ui_parametercombobox.h"

class ParameterComboBox : public ParameterVirtualWidget, Ui::ParameterComboBox
{
	Q_OBJECT

public:
	ParameterComboBox(QWidget *parent, EnumParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onChanged(int index);

private:
	EnumParameter* parameter;
};

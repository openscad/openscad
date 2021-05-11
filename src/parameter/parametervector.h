#pragma once

#include "parametervirtualwidget.h"
#include "ui_parametervector.h"

class ParameterVector : public ParameterVirtualWidget, Ui::ParameterVector
{
	Q_OBJECT

public:
	ParameterVector(QWidget *parent, VectorParameter *parameter, DescriptionStyle descriptionStyle);
	void setValue() override;

protected slots:
	void onChanged();

private:
	VectorParameter* parameter;
	std::vector<QDoubleSpinBox*> spinboxes;
	bool inUpdate = false;
};

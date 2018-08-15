#pragma once

#include "parametervirtualwidget.h"

class ParameterSpinBox :public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterSpinBox(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(double);
	void editingFinished();

private:
	bool volatile suppressUpdate; 
};

#pragma once

#include "parametervirtualwidget.h"

class ParameterSpinBox :public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterSpinBox(ParameterObject *parameterobject, int showDescription);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(double);

private:
	bool volatile suppressUpdate; 
};

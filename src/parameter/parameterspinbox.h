#pragma once

#include "parametervirtualwidget.h"

class ParameterSpinBox :public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterSpinBox(ParameterObject *parameterobject, bool showDescription);
	void setValue();
	void setParameterFocus();

protected slots:
	void onChanged(double);
};

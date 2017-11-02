#pragma once

#include "parametervirtualwidget.h"

class ParameterVector : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterVector(ParameterObject *parameterobject, int showDescription);
	void setValue();
	void setParameterFocus();

protected slots:
	void onChanged(double);
};

#pragma once

#include "parametervirtualwidget.h"

class ParameterCheckBox : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterCheckBox(ParameterObject *parameterobject, int showDescription);
	void setValue();
	void setParameterFocus();

protected slots:
	void onChanged();
};

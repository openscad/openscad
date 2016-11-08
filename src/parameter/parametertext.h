#pragma once

#include "parametervirtualwidget.h"

class ParameterText : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterText(ParameterObject *parameterobject,bool showDescription);
	void setValue();
	void setParameterFocus();

protected slots:
	void onChanged(QString);
};

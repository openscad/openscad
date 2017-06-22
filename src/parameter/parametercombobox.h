#pragma once

#include "parametervirtualwidget.h"

class ParameterComboBox : public ParameterVirtualWidget
{
	Q_OBJECT

public:
	ParameterComboBox(ParameterObject *parameterobject,bool showDescription);
	void setValue();
	void setParameterFocus();

public slots:
	void onChanged(int idx);
};

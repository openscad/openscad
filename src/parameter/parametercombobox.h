#pragma once

#include "parametervirtualwidget.h"

class ParameterComboBox : public ParameterVirtualWidget
{
	Q_OBJECT

public:
	ParameterComboBox(ParameterObject *parameterobject, int showDescription);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(int idx);

private:
	bool volatile suppressUpdate; 
};

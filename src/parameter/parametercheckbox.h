#pragma once

#include "parametervirtualwidget.h"

class ParameterCheckBox : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterCheckBox(ParameterObject *parameterobject, int showDescription);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged();

private:
	bool volatile suppressUpdate; 
};

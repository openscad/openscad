#pragma once

#include "parametervirtualwidget.h"

class ParameterVector : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterVector(ParameterObject *parameterobject, int descriptionLoD);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(double);

private:
	bool volatile suppressUpdate; 
};

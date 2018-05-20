#pragma once

#include "parametervirtualwidget.h"

class ParameterVector : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterVector(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(double);

private:
	bool volatile suppressUpdate; 
};

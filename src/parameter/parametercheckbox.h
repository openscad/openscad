#pragma once

#include "parametervirtualwidget.h"

class ParameterCheckBox : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterCheckBox(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD);
	void setValue() override;

protected slots:
	void onChanged();

private:
	bool volatile suppressUpdate; 
};

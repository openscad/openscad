#pragma once

#include "parametervirtualwidget.h"

class ParameterVector : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterVector(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD);
	void setValue() override;

protected slots:
	void onChanged(double);

private:
	bool volatile suppressUpdate; 
	constexpr static int NR_OF_SPINBOXES = 4;
};

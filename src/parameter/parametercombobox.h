#pragma once

#include "parametervirtualwidget.h"

class ParameterComboBox : public ParameterVirtualWidget
{
	Q_OBJECT

public:
	ParameterComboBox(QWidget *parent, ParameterObject *parameterobject, int descriptionLoD);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(int idx);

private:
	bool volatile suppressUpdate; 
};

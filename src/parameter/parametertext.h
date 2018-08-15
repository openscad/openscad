#pragma once

#include "parametervirtualwidget.h"

class ParameterText : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterText(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(QString);
	void onEditingFinished();

private:
	bool volatile suppressUpdate; 
};

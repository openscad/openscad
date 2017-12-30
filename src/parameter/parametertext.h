#pragma once

#include "parametervirtualwidget.h"

class ParameterText : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterText(ParameterObject *parameterobject, int showDescription);
	void setValue() override;
	void setParameterFocus() override;

protected slots:
	void onChanged(QString);

private:
	bool volatile suppressUpdate; 
};

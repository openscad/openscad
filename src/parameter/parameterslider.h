#pragma once

#include "parametervirtualwidget.h"

class ParameterSlider : public ParameterVirtualWidget
{
	Q_OBJECT
public:
	ParameterSlider(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD);
	void setValue() override;

private:
	double step;
	bool pressed;
	bool volatile suppressUpdate; 

protected slots:
	void onSliderChanged(int);
	void onSpinBoxChanged(double);
	void onReleased();
	void onPressed();
	void onEditingFinished();
};

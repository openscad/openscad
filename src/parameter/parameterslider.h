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
	bool volatile suppressUpdate; 
	static constexpr uint32_t max_uint32 = std::numeric_limits<uint32_t>::max();

protected slots:
	void onSliderChanged(int);
	void onSpinBoxChanged(double);
	void onEditingFinished();
};

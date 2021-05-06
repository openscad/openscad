#pragma once

#include "ui_parameterdescriptionwidget.h"
#include "parameterobject.h"

enum class DescriptionStyle { ShowDetails, Inline, HideDetails, DescriptionOnly };

class ParameterDescriptionWidget : public QWidget, public Ui::ParameterDescriptionWidget
{
	Q_OBJECT

public:
	
	ParameterDescriptionWidget(QWidget *parent);
	void setDescription(ParameterObject *parameter, DescriptionStyle descriptionStyle);
};

class ParameterVirtualWidget : public QWidget
{
	Q_OBJECT

public:
	ParameterVirtualWidget(QWidget *parent, ParameterObject *parameter);
	ParameterObject* getParameter() const { return parameter; }
	virtual void setValue() = 0;

signals:
	void changed();

private:
	ParameterObject* parameter;

protected:
	static int decimalsRequired(double value);
	static int decimalsRequired(const std::vector<double>& values);
};

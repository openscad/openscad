#pragma once

#include "value.h"
#include "qtgettext.h"
#include "ui_ParameterEntryWidget.h"
#include "parameterobject.h"

class ParameterVirtualWidget : public QWidget, public Ui::ParameterEntryWidget
{
	Q_OBJECT

private:
	int LabelWidth=0;

protected:
	ParameterObject *object;

public:
	ParameterVirtualWidget(QWidget *parent = nullptr);
	~ParameterVirtualWidget();
	virtual void setParameterFocus() = 0;
	virtual void setValue() = 0;
	void resizeEvent(QResizeEvent * event) override;

signals:
	void changed();

protected:
	int decimalPrecision;
	virtual void setPrecision(double number);
	void setName(QString name);
	void setDescription(const QString& description);
	void addInline(QString txt);
};

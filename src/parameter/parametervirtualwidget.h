#pragma once

#include "value.h"
#include "qtgettext.h"
#include "ui_ParameterEntryWidget.h"
#include "parameterobject.h"

class ParameterVirtualWidget : public QWidget, public Ui::ParameterEntryWidget
{
	Q_OBJECT
	
protected:
	ParameterObject *object;
	
public:
	ParameterVirtualWidget(QWidget *parent = 0);
	virtual ~ParameterVirtualWidget();
	virtual void setParameterFocus() = 0;
	
signals:
	void changed();
	
protected:
	int decimalPrecision;
	virtual void setPrecision(double number);
	virtual void setValue() = 0;
    void setName(QString name);
	void setDescription(const QString& description) {
		if(!description.isEmpty()){
			this->labelDescription->show();
			this->labelDescription->setText(description);
		}
	}
};

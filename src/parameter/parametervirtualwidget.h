#ifndef PARAMETERVIRTUALWIDGET_H
#define PARAMETERVIRTUALWIDGET_H

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
    ParameterVirtualWidget(QWidget *parent=0) : QWidget(parent)
    {
        setupUi(this);
    }

    virtual ~ParameterVirtualWidget(){}
    virtual void setParameterFocus()=0;

signals:
    void changed();

protected:
    virtual void setValue()=0;
    void setName(const QString& name)
    {
        this->labelDescription->hide();
        this->labelParameter->setText(name);
    }

    void setDescription(const QString& description)
    {
        this->labelDescription->show();
        this->labelDescription->setText(description);
    }
};

#endif // PARAMETERVIRTUALWIDGET_H

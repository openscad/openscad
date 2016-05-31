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

public:
    ParameterObject *object;
    ParameterVirtualWidget(QWidget *parent = 0);
    virtual ~ParameterVirtualWidget();

    ValuePtr getValue();
    bool isDefaultValue();

signals:
    void changed();

protected:
    void setName(const QString& name);
    virtual void setValue()=0;
    void setDescription(const QString& description);
};

#endif // PARAMETERVIRTUALWIDGET_H

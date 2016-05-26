#ifndef PARAMETERSPINBOX_H
#define PARAMETERSPINBOX_H

#include"parametervirtualwidget.h"
#include "parameterobject.h"

class ParameterSpinBox :public ParameterVirtualWidget
{
    Q_OBJECT
public:
    ParameterSpinBox(ParameterObject *parameterobject );
    void setValue();

protected slots:
    void on_Changed(double);
};

#endif // PARAMETERSPINBOX_H

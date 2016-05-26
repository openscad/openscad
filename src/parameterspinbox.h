#ifndef PARAMETERSPINBOX_H
#define PARAMETERSPINBOX_H

#include"parametervirtualwidget.h"
#include "parameterobject.h"

class ParameterSpinBox : public ParameterVirtualWidget
{
public:

    ParameterSpinBox(ParameterObject *parameterobject );
    void setValue();

protected slots:
    void on_doubleSpinBox1_valueChanged(double);
};

#endif // PARAMETERSPINBOX_H

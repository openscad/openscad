#ifndef PARAMETERSLIDER_H
#define PARAMETERSLIDER_H

#include"parametervirtualwidget.h"
#include "parameterobject.h"

class ParameterSlider : public ParameterVirtualWidget
{
         Q_OBJECT
public:
    ParameterSlider(ParameterObject *parameterobject,bool showDescription);
    void setValue();

protected slots:
    void on_Changed(int);
};

#endif // PARAMETERSLIDER_H

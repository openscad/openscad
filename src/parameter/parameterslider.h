#ifndef PARAMETERSLIDER_H
#define PARAMETERSLIDER_H

#include"parametervirtualwidget.h"

class ParameterSlider : public ParameterVirtualWidget
{
         Q_OBJECT
public:
    ParameterSlider(ParameterObject *parameterobject,bool showDescription);
    void setValue();

protected slots:
    void on_Changed();
};

#endif // PARAMETERSLIDER_H

#ifndef PARAMETERSLIDER_H
#define PARAMETERSLIDER_H

#include"parametervirtualwidget.h"

class ParameterSlider : public ParameterVirtualWidget
{
         Q_OBJECT
public:
    ParameterSlider(ParameterObject *parameterobject,bool showDescription);
    void setValue();
    void setParameterFocus();

private:
    double step;

protected slots:
    void onChanged(int);
};

#endif // PARAMETERSLIDER_H

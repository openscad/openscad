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
    bool pressed;
protected slots:
    void onChanged(int);
    void onReleased();
    void onPressed();
};

#endif // PARAMETERSLIDER_H

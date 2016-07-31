#ifndef PARAMETERSLIDER_H
#define PARAMETERSLIDER_H

#include"parametervirtualwidget.h"

class ParameterSlider : public ParameterVirtualWidget
{
         Q_OBJECT
public:
    ParameterSlider(ParameterObject *parameterobject,bool showDescription);
    void setValue();

private:
    int presicion;

protected slots:
    void on_Changed();
    void onMoved(int);
};

#endif // PARAMETERSLIDER_H

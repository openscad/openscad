#ifndef PARAMETERTEXT_H
#define PARAMETERTEXT_H

#include"parametervirtualwidget.h"

class ParameterText : public ParameterVirtualWidget
{
     Q_OBJECT
public:
    ParameterText(ParameterObject *parameterobject,bool showDescription);
    void setValue();

protected slots:
    void on_Changed();
};

#endif // PARAMETERTEXT_H

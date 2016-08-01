#ifndef PARAMETERTEXT_H
#define PARAMETERTEXT_H

#include"parametervirtualwidget.h"

class ParameterText : public ParameterVirtualWidget
{
     Q_OBJECT
public:
    ParameterText(ParameterObject *parameterobject,bool showDescription);
    void setValue();
    void setParameterFocus();

protected slots:
    void onChanged(QString);
};

#endif // PARAMETERTEXT_H

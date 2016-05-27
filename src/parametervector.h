#ifndef PARAMETERVECTOR_H
#define PARAMETERVECTOR_H

#include"parametervirtualwidget.h"

class ParameterVector : public ParameterVirtualWidget
{
    Q_OBJECT
public:
    ParameterVector(ParameterObject *parameterobject);
    void setValue();

protected slots:
    void on_Changed(double);
};

#endif // PARAMETERVECTOR_H

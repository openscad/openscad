#ifndef PARAMETERCHECKBOX_H
#define PARAMETERCHECKBOX_H

#include"parametervirtualwidget.h"
#include "parameterobject.h"

class ParameterCheckBox : public ParameterVirtualWidget
{
    Q_OBJECT
public:
    ParameterCheckBox(ParameterObject *parameterobject);
    void setValue();

protected slots:
    void on_Changed();
};

#endif // PARAMETERCHECKBOX_H

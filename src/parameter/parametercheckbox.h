#ifndef PARAMETERCHECKBOX_H
#define PARAMETERCHECKBOX_H

#include"parametervirtualwidget.h"


class ParameterCheckBox : public ParameterVirtualWidget
{
    Q_OBJECT
public:
    ParameterCheckBox(ParameterObject *parameterobject,bool);
    void setValue();
    void setParameterFocus();

protected slots:
    void onChanged();
};

#endif // PARAMETERCHECKBOX_H

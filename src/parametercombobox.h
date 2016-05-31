#ifndef PARAMETERCOMBOBOX_H
#define PARAMETERCOMBOBOX_H

#include"parametervirtualwidget.h"
#include "parameterobject.h"

class ParameterComboBox : public ParameterVirtualWidget
{
    Q_OBJECT

public:
    ParameterComboBox(ParameterObject *parameterobject,bool showDescription);
    void setValue();

public slots:
    void on_Changed(int idx);
};


#endif // PARAMETERCOMBOBOX_H

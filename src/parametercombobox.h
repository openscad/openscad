#ifndef PARAMETERCOMBOBOX_H
#define PARAMETERCOMBOBOX_H

#include"parametervirtualwidget.h"
#include "parameterobject.h"

class ParameterComboBox : public ParameterVirtualWidget
{
    Q_OBJECT

    QComboBox *spinBox;
public:
    ParameterComboBox(ParameterObject *parameterobject );
    void setValue();

public slots:
    void on_Changed(int idx);
};


#endif // PARAMETERCOMBOBOX_H

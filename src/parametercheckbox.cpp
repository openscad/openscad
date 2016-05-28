#include "parametercheckbox.h"

ParameterCheckBox::ParameterCheckBox(ParameterObject *parameterobject, bool showDescription)
{
    object=parameterobject;
    set();
    connect(checkBox,SIGNAL(clicked()),this,SLOT(on_Changed()));
    if(showDescription==true){
    setDescription(object->description);
    }
    else{
        checkBox->setToolTip(object->description);
    }

}

void ParameterCheckBox::on_Changed(){
    object->value = ValuePtr(checkBox->isChecked());
    emit changed();
}

void ParameterCheckBox::setValue(){
    this->stackedWidget->setCurrentWidget(this->pageCheckBox);
    this->checkBox->setChecked(object->value->toBool());
}

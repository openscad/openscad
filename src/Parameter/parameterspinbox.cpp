#include "parameterspinbox.h"

ParameterSpinBox::ParameterSpinBox(ParameterObject *parameterobject, bool showDescription)
{
    object=parameterobject;
    setName(QString::fromStdString(object->name));
    setValue();
    connect(doubleSpinBox1,SIGNAL(valueChanged(double)),this,SLOT(on_Changed(double)));
    if(showDescription==true){
        setDescription(object->description);
    }
    else{
        doubleSpinBox1->setToolTip(object->description);
    }
}

void ParameterSpinBox::on_Changed(double){

    object->value = ValuePtr(doubleSpinBox1->value());
    emit changed();
}

void ParameterSpinBox::setValue(){

    this->stackedWidget->setCurrentWidget(this->pageVector);
    this->doubleSpinBox1->setValue(object->value->toDouble());
    if(object->values->toDouble()>0){
        this->doubleSpinBox1->setSingleStep(object->values->toDouble());
    }

    this->doubleSpinBox2->hide();
    this->doubleSpinBox3->hide();
    this->doubleSpinBox4->hide();
}

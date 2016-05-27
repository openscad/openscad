#include "parameterspinbox.h"

ParameterSpinBox::ParameterSpinBox(ParameterObject *parameterobject)
{
    object=parameterobject;
    set();
    connect(doubleSpinBox1,SIGNAL(valueChanged(double)),this,SLOT(on_Changed(double)));
}

void ParameterSpinBox::on_Changed(double){
    object->value = ValuePtr(doubleSpinBox1->value());
    emit changed();
}

void ParameterSpinBox::setValue(){
    this->stackedWidget->setCurrentWidget(this->pageVector);
    this->doubleSpinBox1->setValue(object->value->toDouble());
    this->doubleSpinBox2->hide();
    this->doubleSpinBox3->hide();
    this->doubleSpinBox4->hide();
}

#include "parameterslider.h"
#include <sstream>

ParameterSlider::ParameterSlider(ParameterObject *parameterobject, bool showDescription)
{
    presicion =0;
    object=parameterobject;
    setName(QString::fromStdString(object->name));
    setValue();
    connect(slider,SIGNAL(valueChanged(int)),this,SLOT(onChanged(int)));
    if(showDescription==true){
        setDescription(object->description);
    }
    else{
        slider->setToolTip(object->description);
    }
}

void ParameterSlider::onChanged(int)
{
    object->focus=true;
    double v = slider->value()*step;
    this->labelSliderValue->setText(QString::number(v, 'f', presicion));
    object->value = ValuePtr(v);
    emit changed();
}

void ParameterSlider::setParameterFocus()
{
   slider->setFocus();
   object->focus=false;
}


void ParameterSlider::setValue()
{
    std::ostringstream ostr; //output string stream
    ostr <<object->values->toRange().step_value();
    std::string number= ostr.str();
    presicion=number.size()-number.find('.')-1;

    step=object->values->toRange().step_value();

    int min = object->values->toRange().begin_value()/step;
    int max=object->values->toRange().end_value()/step;
    int current=object->value->toDouble()/step;
    this->stackedWidget->setCurrentWidget(this->pageSlider);
    this->slider->setRange(min,max);
    this->slider->setValue(current);
    this->labelSliderValue->setText(QString::number(current*step, 'f',presicion));
}

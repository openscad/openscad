#include "parameterslider.h"
#include <sstream>

ParameterSlider::ParameterSlider(ParameterObject *parameterobject, bool showDescription)
{
    presicion =0;
    object=parameterobject;
    setName(QString::fromStdString(object->name));
    setValue();
    connect(slider,SIGNAL(sliderReleased()),this,SLOT(on_Changed()));
    connect(slider,SIGNAL(valueChanged(int)),this,SLOT(onMoved(int)));
    if(showDescription==true){
        setDescription(object->description);
    }
    else{
        slider->setToolTip(object->description);
    }
}

void ParameterSlider::onMoved(int)
{
    double v = slider->value()/pow(10,presicion);
    this->labelSliderValue->setText(QString::number(v, 'f', presicion));
}

void ParameterSlider::on_Changed()
{
    double v = slider->value()/pow(10,presicion);
    object->value = ValuePtr(v);
    //to be corrected
    this->labelSliderValue->setText(QString::number(v, 'f',presicion));
    emit changed();
}

void ParameterSlider::setValue()
{
    std::ostringstream ostr; //output string stream
    ostr <<object->values->toRange().step_value();
    std::string number= ostr.str();
    presicion=number.size()-number.find('.')-1;
    this->stackedWidget->setCurrentWidget(this->pageSlider);
    this->slider->setRange(object->values->toRange().begin_value()*pow(10,presicion),object->values->toRange().end_value()*pow(10,presicion));
    this->slider->setValue(object->value->toDouble()*pow(10,presicion));
    this->slider->setSingleStep(object->values->toRange().step_value()*pow(10,presicion));
    this->labelSliderValue->setText(QString::number(object->value->toDouble(), 'f',presicion));
}

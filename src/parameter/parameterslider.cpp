#include "parameterslider.h"

ParameterSlider::ParameterSlider(ParameterObject *parameterobject, bool showDescription)
{
    count =0;
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
    double v = slider->value()/pow(10,count);
    this->labelSliderValue->setText(QString::number(v, 'f', count));
}

void ParameterSlider::on_Changed()
{
    double v = slider->value()/pow(10,count);
    object->value = ValuePtr(v);
    //to be corrected
    this->labelSliderValue->setText(QString::number(v, 'f',count));
    emit changed();
}

void ParameterSlider::setValue()
{
    count =0;
    double no =object->values->toRange().step_value();
    while(no!=((int)no))
    {
        count++;
        no=no*10;
    }
    this->stackedWidget->setCurrentWidget(this->pageSlider);
    this->slider->setRange(object->values->toRange().begin_value()*pow(10,count),object->values->toRange().end_value()*pow(10,count));
    this->slider->setValue(object->value->toDouble()*pow(10,count));
    this->slider->setSingleStep(object->values->toRange().step_value()*pow(10,count));
    this->labelSliderValue->setText(QString::number(object->value->toDouble(), 'f',count));
}

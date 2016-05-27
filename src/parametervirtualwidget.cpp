#include "parametervirtualwidget.h"

ParameterVirtualWidget::ParameterVirtualWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
}

ParameterVirtualWidget::~ParameterVirtualWidget()
{

}


ValuePtr ParameterVirtualWidget::getValue()
{
    return object->value;
}

bool ParameterVirtualWidget::isDefaultValue()
{
    return object->value == object->defaultValue;
}

void ParameterVirtualWidget::setName(const QString& name)
{
    this->labelDescription->hide();
    this->labelParameter->setText(name);
}

void ParameterVirtualWidget::setDescription(const QString& description)
{
    this->labelDescription->show();
    this->labelDescription->setText(description);
}

void ParameterVirtualWidget::set(){
    setName(QString::fromStdString(object->name));
    setValue();
    setDescription(QString::fromStdString(object->descritpion));
}

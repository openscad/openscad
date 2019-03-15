#include "parameterspinbox.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterSpinBox::ParameterSpinBox(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	setValue();
	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
	connect(doubleSpinBox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterSpinBox::onChanged(double)
{
	if(!this->suppressUpdate){
		object->value = Value(doubleSpinBox->value());
	}
}

void ParameterSpinBox::onEditingFinished()
{
	emit changed();
}

void ParameterSpinBox::setValue()
{
	this->suppressUpdate=true;
	if (object->values.toDouble() > 0) {
		setPrecision(object->values.toDouble());
		this->doubleSpinBox->setSingleStep(object->values.toDouble());
	}
	else {
		setPrecision(object->defaultValue.toDouble());
		this->doubleSpinBox->setSingleStep(1/pow(10,decimalPrecision));
	}
	this->doubleSpinBox->setDecimals(decimalPrecision);
	this->stackedWidgetRight->setCurrentWidget(this->pageSpin);
	this->pageSpin->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->stackedWidgetBelow->hide();
	this->doubleSpinBox->setRange(object->value.toDouble()-1000, object->value.toDouble()+1000);
	this->doubleSpinBox->setValue(object->value.toDouble());
	this->suppressUpdate=false;
}

#include "parameterspinbox.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterSpinBox::ParameterSpinBox(ParameterObject *parameterobject, int showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
	if (showDescription == 0) {
		setDescription(object->description);
	}else if(showDescription == 1){
		addInline(object->description);
	}else {
		doubleSpinBox->setToolTip(object->description);
	}
	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused();
	doubleSpinBox->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterSpinBox::onChanged(double)
{
	if(!suppressUpdate){
		object->focus = true;
		object->value = ValuePtr(doubleSpinBox->value());
		emit changed();
	}
}

void ParameterSpinBox::setParameterFocus()
{
	this->doubleSpinBox->setFocus();
	object->focus = false;
}

void ParameterSpinBox::setValue()
{
	if(hasFocus())return; //refuse programmatic updates, when the widget is in the focus of the user

	suppressUpdate=true;
	if (object->values->toDouble() > 0) {
		setPrecision(object->values->toDouble());
		this->doubleSpinBox->setSingleStep(object->values->toDouble());
	}
	else {
		setPrecision(object->defaultValue->toDouble());
		this->doubleSpinBox->setSingleStep(1/pow(10,decimalPrecision));
	}
	this->doubleSpinBox->setDecimals(decimalPrecision);
	this->stackedWidgetRight->setCurrentWidget(this->pageSpin);
	this->pageSpin->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->stackedWidgetBelow->hide();
	this->doubleSpinBox->setRange(object->value->toDouble()-1000, object->value->toDouble()+1000);
	this->doubleSpinBox->setValue(object->value->toDouble());
	suppressUpdate=false;
}

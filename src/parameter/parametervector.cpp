#include "parametervector.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterVector::ParameterVector(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	setValue();
	connect(doubleSpinBox1,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox2,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox3,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox4,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	this->doubleSpinBox1->installEventFilter(ignoreWheelWhenNotFocused);
	this->doubleSpinBox2->installEventFilter(ignoreWheelWhenNotFocused);
	this->doubleSpinBox3->installEventFilter(ignoreWheelWhenNotFocused);
	this->doubleSpinBox4->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterVector::onChanged(double)
{
	if(!this->suppressUpdate){
		if (object->target == ParameterObject::NUMBER) {
			object->value = ValuePtr(doubleSpinBox1->value());
		} else {
			Value::VectorType vt;
			vt.push_back(this->doubleSpinBox1->value());
			if (!this->doubleSpinBox2->isReadOnly()) {
				vt.push_back(this->doubleSpinBox2->value());
			}
			if (!this->doubleSpinBox3->isReadOnly()) {
				vt.push_back(this->doubleSpinBox3->value());
			}
			if (!this->doubleSpinBox4->isReadOnly()) {
				vt.push_back(this->doubleSpinBox4->value());
			}
			object->value = ValuePtr(vt);
		}
		emit changed();
	}
}

void ParameterVector::setValue()
{
	this->suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageVector);
	this->pageVector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();

	Value::VectorType vec = object->value->toVector();

	double minV = object->values->toRange().begin_value();
	double step = object->values->toRange().step_value();
	double maxV = object->values->toRange().end_value();
	if(step==0){
		step=1;
		setPrecision(vec.at(0)->toDouble());
	}else{
		setPrecision(step);
	}

	QDoubleSpinBox* boxes[4] = {this->doubleSpinBox1,this->doubleSpinBox2,this->doubleSpinBox3,this->doubleSpinBox4};

	for(unsigned int i = 0; i < vec.size(); i++) {
		boxes[i]->setDecimals(decimalPrecision);
		boxes[i]->setValue(vec.at(i)->toDouble());
		if(minV==0 && maxV ==0){
			boxes[i]->setRange(vec.at(i)->toDouble()-1000,vec.at(i)->toDouble()+1000);
		}else{
			boxes[i]->setMinimum(minV);
			boxes[i]->setMaximum(maxV);
			boxes[i]->setSingleStep(step);
		}
	}
	for(unsigned int i = vec.size(); i < 4; i++) {
		boxes[i]->hide();
		boxes[i]->setReadOnly(true);
	}
	this->suppressUpdate=false;
}

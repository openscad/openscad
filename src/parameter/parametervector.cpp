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
			object->value = Value(doubleSpinBox1->value());
		} else {
			Value::VectorPtr vt;
			vt->emplace_back(this->doubleSpinBox1->value());
			if (!this->doubleSpinBox2->isReadOnly()) {
				vt->emplace_back(this->doubleSpinBox2->value());
			}
			if (!this->doubleSpinBox3->isReadOnly()) {
				vt->emplace_back(this->doubleSpinBox3->value());
			}
			if (!this->doubleSpinBox4->isReadOnly()) {
				vt->emplace_back(this->doubleSpinBox4->value());
			}
			object->value = Value(vt);
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

	const Value::VectorPtr &vec = object->value.toVectorPtr();

	double minV = object->values.toRange().begin_value();
	double step = object->values.toRange().step_value();
	double maxV = object->values.toRange().end_value();
	if(step==0){
		step=1;
		setPrecision(vec->at(0).toDouble());
	}else{
		setPrecision(step);
	}

	QDoubleSpinBox* boxes[NR_OF_SPINBOXES] = {this->doubleSpinBox1,this->doubleSpinBox2,this->doubleSpinBox3,this->doubleSpinBox4};

	for(unsigned int i = 0; i < vec->size() && i < NR_OF_SPINBOXES; i++) {
		boxes[i]->show();
		boxes[i]->setDecimals(decimalPrecision);
		if(minV==0 && maxV ==0){
			boxes[i]->setRange(vec->at(i).toDouble()-1000,vec->at(i).toDouble()+1000);
		}else{
			boxes[i]->setMinimum(minV);
			boxes[i]->setMaximum(maxV);
			boxes[i]->setSingleStep(step);
		}
		boxes[i]->setValue(vec->at(i).toDouble());
	}
	for(unsigned int i = vec->size(); i < NR_OF_SPINBOXES; i++) {
		boxes[i]->hide();
		boxes[i]->setReadOnly(true);
	}
	this->suppressUpdate=false;
}

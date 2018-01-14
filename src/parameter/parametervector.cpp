#include "parametervector.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterVector::ParameterVector(ParameterObject *parameterobject, int showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(doubleSpinBox1,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox2,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox3,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	connect(doubleSpinBox4,SIGNAL(valueChanged(double)),this,SLOT(onChanged(double)));
	if (showDescription == 0) {
		setDescription(object->description);
	}else if(showDescription == 1){
		addInline(object->description);
	}else {
		this->setToolTip(object->description);
	}

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused();
	doubleSpinBox1->installEventFilter(ignoreWheelWhenNotFocused);
	doubleSpinBox2->installEventFilter(ignoreWheelWhenNotFocused);
	doubleSpinBox3->installEventFilter(ignoreWheelWhenNotFocused);
	doubleSpinBox4->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterVector::onChanged(double)
{
	if(!suppressUpdate){
		object->focus = true;
		if (object->target == 5) {
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

void ParameterVector::setParameterFocus()
{
	this->doubleSpinBox1->setFocus();
	object->focus = false;
}

void ParameterVector::setValue()
{
	if(hasFocus())return; //refuse programmatic updates, when the widget is in the focus of the user

	suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageVector);
	this->pageVector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();

	double minV = object->values->toRange().begin_value();
	double step = object->values->toRange().step_value();
	double maxV = object->values->toRange().end_value();
	if(step==0){
		step=1;
	}

	QDoubleSpinBox* boxes[4] = {this->doubleSpinBox1,this->doubleSpinBox2,this->doubleSpinBox3,this->doubleSpinBox4};
	Value::VectorType vec = object->value->toVector();
	setPrecision(vec.at(0)->toDouble());
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
	suppressUpdate=false;
}

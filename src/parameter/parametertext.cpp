#include "parametertext.h"
#include "modcontext.h"
#include "comment.h"

ParameterText::ParameterText(ParameterObject *parameterobject, int showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();

	double max=32767;
	if(object->values->type() == Value::ValueType::RANGE ){ // [min:max] and [min:step:max] format;
		max = object->values->toRange().end_value();
	}else if(object->values->toVector().size() == 1){ // [max] format from makerbot customizer
		max = std::stoi(object->values->toVector()[0]->toString(),nullptr,0);
	}
	lineEdit->setMaxLength(max);

	connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onChanged(QString)));
	if (showDescription == 0 || showDescription == 3) {
		setDescription(object->description);
		this->labelInline->hide();
	}else if(showDescription == 1){
		addInline(object->description);
	}else{
		lineEdit->setToolTip(object->description);
	}

	if (showDescription == 3 && object->description !=""){
		labelParameter->hide();
	}else{
		labelParameter->show();
	}
}

void ParameterText::onChanged(QString)
{
	if(!suppressUpdate){
		if (object->dvt == Value::ValueType::STRING) {
			object->value = ValuePtr(lineEdit->text().toStdString());
		}else{
			ModuleContext ctx;
			shared_ptr<Expression> params = CommentParser::parser(lineEdit->text().toStdString().c_str());
			if (!params) return;
			ValuePtr newValue = params->evaluate(&ctx);
			if (object->dvt == newValue->type()) {
				object->value = newValue;
			}
		}
		object->focus = true;
		emit changed();
	}
}

void ParameterText::setParameterFocus()
{
	this->lineEdit->setFocus();
	object->focus = false;
}

void ParameterText::setValue()
{
	suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageText);
	this->pageText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();
	this->lineEdit->setText(QString::fromStdString(object->value->toString()));
	if (object->values->toDouble() > 0) {
		this->lineEdit->setMaxLength(object->values->toDouble());
	}
	suppressUpdate=false;
}

#include "parametertext.h"
#include "modcontext.h"
#include "comment.h"

ParameterText::ParameterText(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	setValue();

	double max=32767;
	if(object->values->toVector().size() == 1){ // [max] format from makerbot customizer
		max = std::stoi(object->values->toVector()[0]->toString(),nullptr,0);
	}
	lineEdit->setMaxLength(max);

	connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onChanged(QString)));
	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
}

void ParameterText::onChanged(QString)
{
	if(!this->suppressUpdate){
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
		
	}
}

void ParameterText::editingFinished()
{
	emit changed();
}

void ParameterText::setParameterFocus()
{
	this->lineEdit->setFocus();
	object->focus = false;
}

void ParameterText::setValue()
{
	this->suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageText);
	this->pageText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();
	this->lineEdit->setText(QString::fromStdString(object->value->toString()));
	if (object->values->toDouble() > 0) {
		this->lineEdit->setMaxLength(object->values->toDouble());
	}
	this->suppressUpdate=false;
}

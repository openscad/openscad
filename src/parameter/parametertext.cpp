#include "parametertext.h"
#include "modcontext.h"
#include "comment.h"

ParameterText::ParameterText(ParameterObject *parameterobject, int showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onChanged(QString)));
	if (showDescription == 0) {
		setDescription(object->description);
	}else if(showDescription == 1){
		addInline(object->description);
	}else {
		lineEdit->setToolTip(object->description);
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

#include "parametertext.h"
#include "modcontext.h"
#include "comment.h"

ParameterText::ParameterText(ParameterObject *parameterobject, bool showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onChanged(QString)));
	if (showDescription == true){
		setDescription(object->description);
	}
	else{
		lineEdit->setToolTip(object->description);
	}
}

void ParameterText::onChanged(QString)
{
	if (object->dvt == Value::STRING) {
		object->value = ValuePtr(lineEdit->text().toStdString());
	}
	else {
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

void ParameterText::setParameterFocus()
{
	this->lineEdit->setFocus();
	object->focus = false;
}

void ParameterText::setValue()
{
	this->stackedWidget->setCurrentWidget(this->pageText);
	this->lineEdit->setText(QString::fromStdString(object->value->toString()));
	if (object->values->toDouble() > 0) {
		this->lineEdit->setMaxLength(object->values->toDouble());
	}
}

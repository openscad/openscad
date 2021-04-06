#include "parametertext.h"
#include "modcontext.h"
#include "comment.h"

ParameterText::ParameterText(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	setValue();

	double max = 32767;
	const auto &values = object->values.toVector();
	if(values.size() == 1) { // [max] format from makerbot customizer
		try {
			max = std::stoi(values[0].toString(), nullptr, 0);
		}
		catch(...) { } // If not a valid value, fall back to the default.
	}
	this->lineEdit->setMaxLength(max);

	connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onChanged(QString)));
	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
}

void ParameterText::onChanged(QString)
{
	if(!this->suppressUpdate){
		if (object->dvt == Value::Type::STRING) {
			object->value = Value(lineEdit->text().toStdString());
		}else{
			ContextHandle<Context> ctx{Context::create<Context>()};
			shared_ptr<Expression> params = CommentParser::parser(lineEdit->text().toStdString().c_str());
			if (!params) return;
			Value newValue = params->evaluate(ctx.ctx);
			if (object->dvt == newValue.type()) {
				object->value = std::move(newValue);
			}
		}
	}
}

void ParameterText::onEditingFinished()
{
	emit changed();
}

void ParameterText::setValue()
{
	this->suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageText);
	this->pageText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();
	this->lineEdit->setText(QString::fromStdString(object->value.toString()));
	if (object->values.toDouble() > 0) {
		this->lineEdit->setMaxLength(object->values.toDouble());
	}
	this->suppressUpdate=false;
}

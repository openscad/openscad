#include "parametertext.h"

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
}

void ParameterText::onChanged(QString)
{
	if(!this->suppressUpdate){
		object->value = Value(lineEdit->text().toStdString());
		emit changed();
	}
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

#include "parametercheckbox.h"

ParameterCheckBox::ParameterCheckBox(QWidget *parent, ParameterObject *parameterobject, DescLoD descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	setValue();
	connect(checkBox, SIGNAL(clicked()), this, SLOT(onChanged()));

	if (descriptionLoD == DescLoD::ShowDetails){
		//large checkbox, when we have the space
		checkBox->setStyleSheet("QCheckBox::indicator {\nwidth: 20px;\nheight: 20px;\n}");
	}
}

void ParameterCheckBox::onChanged()
{
	if(!this->suppressUpdate){
		object->value = Value(checkBox->isChecked());
		emit changed();
	}
}

void ParameterCheckBox::setValue() {
	this->suppressUpdate=true;
	this->stackedWidgetRight->setCurrentWidget(this->pageCheckBox);
	this->pageCheckBox->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->stackedWidgetBelow->hide();
	this->checkBox->setChecked(object->value.toBool());
	this->suppressUpdate=false;
}

#include "parametercheckbox.h"

ParameterCheckBox::ParameterCheckBox(ParameterObject *parameterobject, int showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(checkBox, SIGNAL(clicked()), this, SLOT(onChanged()));

	if (showDescription == 0){
		setDescription(object->description);
	}else if(showDescription == 1){
		addInline(object->description);
		checkBox->setStyleSheet(""); //small checkbox, when description not shown
	}else{
		checkBox->setToolTip(object->description);
		checkBox->setStyleSheet(""); //small checkbox, when description not shown
	}
}

void ParameterCheckBox::onChanged()
{
	if(!suppressUpdate){
		object->focus = true;
		object->value = ValuePtr(checkBox->isChecked());
		emit changed();
	}
}

void ParameterCheckBox::setParameterFocus()
{
	this->checkBox->setFocus();
	object->focus = false;
}

void ParameterCheckBox::setValue() {
	suppressUpdate=true;
	this->stackedWidgetRight->setCurrentWidget(this->pageCheckBox);
	this->pageCheckBox->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
	this->stackedWidgetBelow->hide();
	this->checkBox->setChecked(object->value->toBool());
	suppressUpdate=false;
}

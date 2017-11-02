#include "parametercheckbox.h"

ParameterCheckBox::ParameterCheckBox(ParameterObject *parameterobject, bool showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(checkBox, SIGNAL(clicked()), this, SLOT(onChanged()));
	if (showDescription == true) {
		setDescription(object->description);
	}
	else {
		checkBox->setToolTip(object->description);
		checkBox->setStyleSheet(""); //small checkbox, when description not shown
	}
}

void ParameterCheckBox::onChanged()
{
	object->focus = true;
	object->value = ValuePtr(checkBox->isChecked());
	emit changed();
}

void ParameterCheckBox::setParameterFocus()
{
	this->checkBox->setFocus();
	object->focus = false;
}

void ParameterCheckBox::setValue() {
	this->stackedWidgetRight->setCurrentWidget(this->pageCheckBox);
	this->stackedWidgetBelow->hide();
	this->checkBox->setChecked(object->value->toBool());
}

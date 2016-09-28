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
	this->stackedWidget->setCurrentWidget(this->pageCheckBox);
	this->checkBox->setChecked(object->value->toBool());
}

#include "parametercheckbox.h"

ParameterCheckBox::ParameterCheckBox(QWidget *parent, ParameterObject *parameterobject, int descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(checkBox, SIGNAL(clicked()), this, SLOT(onChanged()));
	if (descriptionLoD == descLoD::ShowDetails || descriptionLoD == descLoD::DescOnly) {
		setDescription(object->description);
		this->labelInline->hide();
	}else if(descriptionLoD == descLoD::Inline){
		addInline(object->description);
	}else{
		this->setToolTip(object->description);
	}

	if (descriptionLoD == descLoD::DescOnly && object->description !=""){
		labelParameter->hide();
	}else{
		labelParameter->show();
	}

	if (!descriptionLoD == descLoD::ShowDetails){
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

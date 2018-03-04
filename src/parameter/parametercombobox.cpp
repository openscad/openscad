#include "parametercombobox.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterComboBox::ParameterComboBox(QWidget *parent, ParameterObject *parameterobject, int descriptionLoD)
	: ParameterVirtualWidget(parent, parameterobject, descriptionLoD)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
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

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	comboBox->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterComboBox::onChanged(int idx)
{
	if(!suppressUpdate){
		if (object->dvt == Value::ValueType::STRING) {
			const std::string v = comboBox->itemData(idx).toString().toStdString();
			object->value = ValuePtr(v);
		} else {
			const double v = comboBox->itemData(idx).toDouble();
			object->value = ValuePtr(v);
		}
		object->focus = true;
		emit changed();
	}
}

void ParameterComboBox::setParameterFocus()
{
	this->comboBox->setFocus();
	object->focus = false;
}

void ParameterComboBox::setValue()
{
	suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageComboBox);
	this->pageComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();
	comboBox->clear();
	const Value::VectorType& vec = object->values->toVector();
	for (Value::VectorType::const_iterator it = vec.begin(); it != vec.end(); it++)	{
		if ((*it)->toVector().size() > 1) {
			comboBox->addItem(QString::fromStdString((*it)->toVector()[1]->toString()),
												QVariant(QString::fromStdString((*it)->toVector()[0]->toString())));
		}
		else {
			comboBox->addItem(QString::fromStdString((*it)->toString()),
												QVariant(QString::fromStdString((*it)->toString())));
			
		}
	}
	QString defaultText = QString::fromStdString(object->value->toString());
	int idx = comboBox->findData(QVariant(defaultText));
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	}
	suppressUpdate=false;
}

#include "parametercombobox.h"

ParameterComboBox::ParameterComboBox(ParameterObject *parameterobject, bool showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
	if (showDescription == true) {
		setDescription(object->description);
	}
	else{
		comboBox->setToolTip(object->description);
	}
}

void ParameterComboBox::onChanged(int idx)
{
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

void ParameterComboBox::setParameterFocus()
{
	this->comboBox->setFocus();
	object->focus = false;
}

void ParameterComboBox::setValue()
{
	this->stackedWidget->setCurrentWidget(this->pageComboBox);
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
}

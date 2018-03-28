#include "parametercombobox.h"
#include "ignoreWheelWhenNotFocused.h"

ParameterComboBox::ParameterComboBox(ParameterObject *parameterobject, int showDescription)
{
	object = parameterobject;
	setName(QString::fromStdString(object->name));
	setValue();
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
	if (showDescription == 0 || showDescription == 3) {
		setDescription(object->description);
		this->labelInline->hide();
	}else if(showDescription == 1){
		addInline(object->description);
	}else {
		comboBox->setToolTip(object->description);
	}

	if (showDescription == 3 && object->description !=""){
		this->labelParameter->hide();
	}else{
		this->labelParameter->show();
	}

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	comboBox->installEventFilter(ignoreWheelWhenNotFocused);
}

void ParameterComboBox::onChanged(int idx)
{
	if(!this->suppressUpdate){
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
	this->suppressUpdate=true;
	this->stackedWidgetBelow->setCurrentWidget(this->pageComboBox);
	this->pageComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	this->stackedWidgetRight->hide();
	comboBox->clear();
	const Value::VectorType& vec = object->values->toVector();
	for (const auto &it : vec) {
		QString text, data;
		if ((*it).toVector().size() > 1) {
			text = QString::fromStdString((*it).toVector()[1]->toString());
			data = QString::fromStdString((*it).toVector()[0]->toString());
		} else {
			text = QString::fromStdString((*it).toString());
			data = QString::fromStdString((*it).toString());
			
		}
		comboBox->addItem(text, QVariant(data));
	}
	QString defaultText = QString::fromStdString(object->value->toString());
	int idx = comboBox->findData(QVariant(defaultText));
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	}
	this->suppressUpdate=false;
}

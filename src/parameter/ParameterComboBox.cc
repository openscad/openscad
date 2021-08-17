#include "ParameterComboBox.h"
#include "IgnoreWheelWhenNotFocused.h"

ParameterComboBox::ParameterComboBox(QWidget *parent, EnumParameter* parameter, DescriptionStyle descriptionStyle):
	ParameterVirtualWidget(parent, parameter),
	parameter(parameter)
{
	setupUi(this);
	descriptionWidget->setDescription(parameter, descriptionStyle);

	IgnoreWheelWhenNotFocused *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(this);
	comboBox->installEventFilter(ignoreWheelWhenNotFocused);

	for (const auto& item : parameter->items) {
		comboBox->addItem(QString::fromStdString(item.key));
	}

	connect(comboBox, SIGNAL(activated(int)), this, SLOT(onChanged(int)));
	setValue();
}

void ParameterComboBox::onChanged(int index)
{
	if (parameter->valueIndex != index) {
		parameter->valueIndex	= index;
		emit changed(true);
	}
}

void ParameterComboBox::setValue()
{
	comboBox->setCurrentIndex(parameter->valueIndex);
}

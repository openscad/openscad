#include "parametertext.h"

ParameterText::ParameterText(QWidget *parent, StringParameter *parameter, DescriptionStyle descriptionStyle):
	ParameterVirtualWidget(parent, parameter),
	parameter(parameter)
{
	setupUi(this);
	descriptionWidget->setDescription(parameter, descriptionStyle);

	if (parameter->maximumSize) {
		lineEdit->setMaxLength(*parameter->maximumSize);
	}

	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(onChanged()));
	setValue();
}

void ParameterText::onChanged()
{
	parameter->value = lineEdit->text().toStdString();
	emit changed();
}

void ParameterText::setValue()
{
	lineEdit->setText(QString::fromStdString(parameter->value));
}

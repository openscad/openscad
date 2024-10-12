#include "gui/parameter/ParameterText.h"

#include <QString>
#include <QWidget>
#include <string>

ParameterText::ParameterText(QWidget *parent, StringParameter *parameter, DescriptionStyle descriptionStyle) :
  ParameterVirtualWidget(parent, parameter),
  parameter(parameter)
{
  setupUi(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);

  if (parameter->maximumSize) {
    lineEdit->setMaxLength(*parameter->maximumSize);
  }

  connect(lineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onEdit(const QString&)));
  connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  ParameterText::setValue();
}

void ParameterText::valueApplied() {
  lastApplied = lastSent;
}

void ParameterText::onEdit(const QString& text)
{
#ifdef DEBUG
  PRINTD("edit");
#endif
  std::string value = text.toStdString();
  if (lastSent != value) {
    lastSent = parameter->value = value;
    emit changed(false);
  }
}

void ParameterText::onEditingFinished() {
#ifdef DEBUG
  PRINTD("editing finished");
#endif
  if (lastApplied != parameter->value) {
    lastSent = parameter->value = lineEdit->text().toStdString();
    emit changed(true);
  }
}

void ParameterText::setValue()
{
  lastApplied = lastSent = parameter->value;
  lineEdit->setText(QString::fromStdString(parameter->value));
}

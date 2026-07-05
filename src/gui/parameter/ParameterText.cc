#include "gui/parameter/ParameterText.h"

#include <QString>
#include <QWidget>
#include <string>

#include "gui/parameter/ParameterVirtualWidget.h"

ParameterText::ParameterText(QWidget *parent, StringParameter *parameter,
                             DescriptionStyle descriptionStyle)
  : ParameterVirtualWidget(parent, parameter), parameter(parameter)
{
  setupUi(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);

  if (parameter->maximumSize) {
    lineEdit->setMaxLength(*parameter->maximumSize);
  }

  connect(lineEdit, &QLineEdit::textEdited, this, &ParameterText::onEdit);
  connect(lineEdit, &QLineEdit::editingFinished, this, &ParameterText::onEditingFinished);
  ParameterText::setValue();
}

void ParameterText::valueApplied()
{
  lastApplied = parameter->value;
}

void ParameterText::onEdit(const QString& text)
{
#ifdef DEBUG
  PRINTD("edit");
#endif
  std::string value = text.toStdString();
  if (lastSent != value) {
    lastSent = value;
  }
}

void ParameterText::onEditingFinished()
{
#ifdef DEBUG
  PRINTD("editing finished");
#endif
  std::string value = lineEdit->text().toStdString();
  if (lastApplied != value) {
    lastSent = parameter->value = value;
    emit changed(true);
  }
}

void ParameterText::setValue()
{
  lastApplied = lastSent = parameter->value;
  lineEdit->setText(QString::fromStdString(parameter->value));
}

#include "gui/parameter/ParameterVirtualWidget.h"

#include <QSizePolicy>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <QRegularExpression>
#include <vector>

ParameterDescriptionWidget::ParameterDescriptionWidget(QWidget *parent) :
  QWidget(parent)
{
  setupUi(this);
}

void ParameterDescriptionWidget::setDescription(ParameterObject *parameter, DescriptionStyle descriptionStyle)
{
  labelParameter->setText(QString::fromStdString(parameter->name()).replace(QRegularExpression("([_]+)"), " "));
  if (parameter->description().empty()) {
    labelDescription->hide();
    labelInline->setText("");
  } else if (descriptionStyle == DescriptionStyle::ShowDetails) {
    labelDescription->setText(QString::fromStdString(parameter->description()));
    labelInline->setText("");
  } else if (descriptionStyle == DescriptionStyle::Inline) {
    labelDescription->hide();
    labelInline->setText(QString(" - ") + QString::fromStdString(parameter->description()));
  } else if (descriptionStyle == DescriptionStyle::HideDetails) {
    labelDescription->hide();
    labelInline->setText("");
    this->setToolTip(QString::fromStdString(parameter->description()));
  } else if (descriptionStyle == DescriptionStyle::DescriptionOnly) {
    labelParameter->hide();
    labelDescription->setText(QString::fromStdString(parameter->description()));
    labelInline->hide();
  } else {
    assert(false);
  }
}

ParameterVirtualWidget::ParameterVirtualWidget(QWidget *parent, ParameterObject *parameter) :
  QWidget(parent),
  parameter(parameter)
{
  QSizePolicy policy;
  policy.setHorizontalPolicy(QSizePolicy::Ignored);
  policy.setVerticalPolicy(QSizePolicy::Maximum);
  policy.setHorizontalStretch(0);
  policy.setVerticalStretch(0);
  this->setSizePolicy(policy);

  setContentsMargins(4, 0, 4, 0);
}

int ParameterVirtualWidget::decimalsRequired(double value)
{
  int decimals = 0;
  value = std::abs(value);
  while (std::floor(value) < std::ceil(value) && decimals < 7) {
    decimals++;
    value *= 10.0;
  }
  return decimals;
}

int ParameterVirtualWidget::decimalsRequired(const std::vector<double>& values)
{
  assert(!values.empty());
  int decimals = 0;
  for (double value : values) {
    decimals = std::max(decimals, decimalsRequired(value));
  }
  return decimals;
}

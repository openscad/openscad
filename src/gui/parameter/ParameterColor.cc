#include "gui/parameter/ParameterColor.h"
#include "gui/qtgettext.h"
#include "core/customizer/ParameterObject.h"
#include <QColorDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

ParameterColor::ParameterColor(QWidget *parent, ColorParameter *parameter,
                               DescriptionStyle descriptionStyle)
  : ParameterVirtualWidget(parent, parameter), parameter(parameter)
{
  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  descriptionWidget = new ParameterDescriptionWidget(this);
  descriptionWidget->setDescription(parameter, descriptionStyle);
  rootLayout->addWidget(descriptionWidget);

  auto *contentLayout = new QHBoxLayout();
  contentLayout->setContentsMargins(0, 2, 0, 2);

  colorButton = new QPushButton(this);
  colorButton->setMinimumHeight(30);
  colorButton->setCursor(Qt::PointingHandCursor);
  connect(colorButton, &QPushButton::clicked, this, &ParameterColor::onClicked);

  contentLayout->addWidget(colorButton);
  rootLayout->addLayout(contentLayout);

  ParameterColor::setValue();
}

void ParameterColor::onClicked()
{
  QColor initial;
  if (parameter->format == ColorParameter::ColorFormat::Hex) {
    initial = QColor(QString::fromStdString(parameter->stringValue));
  } else {
    auto& v = parameter->vectorValue;
    if (v.size() >= 3) {
      int r = static_cast<int>(v[0] * 255);
      int g = static_cast<int>(v[1] * 255);
      int b = static_cast<int>(v[2] * 255);
      int a = (v.size() == 4) ? static_cast<int>(v[3] * 255) : 255;
      initial = QColor(r, g, b, a);
    }
  }

  QColor selected =
    QColorDialog::getColor(initial, this, _("Select Color"), QColorDialog::ShowAlphaChannel);
  if (selected.isValid()) {
    if (parameter->format == ColorParameter::ColorFormat::Hex) {
      parameter->stringValue = selected.name(QColor::HexArgb).toStdString();
    } else {
      parameter->vectorValue.clear();
      parameter->vectorValue.push_back(selected.redF());
      parameter->vectorValue.push_back(selected.greenF());
      parameter->vectorValue.push_back(selected.blueF());
      if (parameter->vectorValue.size() == 4) {
        parameter->vectorValue.push_back(selected.alphaF());
      }
    }
    updateButtonColor();
    emit changed(true);  // Immediate update
  }
}

void ParameterColor::setValue()
{
  updateButtonColor();
}

void ParameterColor::updateButtonColor()
{
  QColor color;
  QString text;
  if (parameter->format == ColorParameter::ColorFormat::Hex) {
    color = QColor(QString::fromStdString(parameter->stringValue));
    text = QString::fromStdString(parameter->stringValue);
  } else {
    auto& v = parameter->vectorValue;
    if (v.size() >= 3) {
      int r = static_cast<int>(v[0] * 255);
      int g = static_cast<int>(v[1] * 255);
      int b = static_cast<int>(v[2] * 255);
      int a = (v.size() == 4) ? static_cast<int>(v[3] * 255) : 255;
      color = QColor(r, g, b, a);
      text = QString("[%1, %2, %3%4]")
               .arg(v[0])
               .arg(v[1])
               .arg(v[2])
               .arg(v.size() == 4 ? QString(", %1").arg(v[3]) : "");
    }
  }

  if (color.isValid()) {
    QString style = QString(
                      "background-color: %1; color: %2; border: 1px solid #999; border-radius: 4px; "
                      "font-weight: bold;")
                      .arg(color.name())
                      .arg(color.lightness() > 128 ? "black" : "white");
    colorButton->setStyleSheet(style);
    colorButton->setText(text);
  }
}

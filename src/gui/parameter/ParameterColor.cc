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
    QString hex = QString::fromStdString(parameter->stringValue);
    if (hex.startsWith('#')) {
      bool ok;
      if (hex.length() == 7) {
        initial = QColor(hex);
      } else if (hex.length() == 9) {
        // OpenSCAD #RRGGBBAA -> Qt #AARRGGBB
        unsigned int val = hex.mid(1).toUInt(&ok, 16);
        if (ok) {
          int r = (val >> 24) & 0xFF;
          int g = (val >> 16) & 0xFF;
          int b = (val >> 8) & 0xFF;
          int a = val & 0xFF;
          initial = QColor(r, g, b, a);
        }
      }
    }
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
      QString hex = QString("#%1%2%3")
                      .arg(selected.red(), 2, 16, QChar('0'))
                      .arg(selected.green(), 2, 16, QChar('0'))
                      .arg(selected.blue(), 2, 16, QChar('0'));
      if (selected.alpha() < 255) {
        hex += QString("%1").arg(selected.alpha(), 2, 16, QChar('0'));
      }
      parameter->stringValue = hex.toStdString();
    } else {
      parameter->vectorValue.clear();
      parameter->vectorValue.push_back(selected.redF());
      parameter->vectorValue.push_back(selected.greenF());
      parameter->vectorValue.push_back(selected.blueF());
      if (selected.alpha() < 255) {
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
    QString hex = QString::fromStdString(parameter->stringValue);
    if (hex.length() == 9) {
      bool ok;
      unsigned int val = hex.mid(1).toUInt(&ok, 16);
      if (ok) {
        color = QColor((val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
      }
    } else {
      color = QColor(hex);
    }
    text = hex;
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
    // Buttons don't usually show transparency well, but we can at least ensure
    // the label color is correct based on the "flat" version of the color.
    QColor opaqueColor = color;
    opaqueColor.setAlpha(255);
    QString style = QString(
                      "background-color: %1; color: %2; border: 1px solid #999; border-radius: 4px; "
                      "font-weight: bold;")
                      .arg(color.name(QColor::HexRgb))
                      .arg(opaqueColor.lightness() > 128 ? "black" : "white");
    colorButton->setStyleSheet(style);
    colorButton->setText(text);
  }
}

#include "ExportGcodeDialog.h"
#include <QColorDialog>
#include <QPushButton>

ExportGcodeDialog::ExportGcodeDialog()
{
  setupUi(this);
//  fillColor = QColor(Qt::white);
//  strokeColor = QColor(Qt::black);
//  doubleSpinBoxStrokeWidth->setValue(defaultStrokeWidth);
//  updateFillColor(fillColor);
//  updateStrokeColor(strokeColor);
//  updateFillControlsEnabled();
//  updateStrokeControlsEnabled();
}

int ExportGcodeDialog::exec()
{
  return QDialog::exec();
}
/*
QColor ExportGcodeDialog::getFillColor() const
{
  return fillColor;
}

bool ExportGcodeDialog::isFillEnabled() const
{
  return checkBoxEnableFill->isChecked();
}

QColor ExportGcodeDialog::getStrokeColor() const
{
  return strokeColor;
}

bool ExportGcodeDialog::isStrokeEnabled() const
{
  return checkBoxEnableStroke->isChecked();
}

double ExportGcodeDialog::getStrokeWidth() const
{
  return doubleSpinBoxStrokeWidth->value();
}
*/
ExportGcodeOptions ExportGcodeDialog::getOptions() const
{
  ExportGcodeOptions opts;
//  opts.fill = isFillEnabled();
//  opts.fillColor = getFillColor().name(QColor::HexRgb).toStdString();
//  opts.stroke = isStrokeEnabled();
//  opts.strokeColor = getStrokeColor().name(QColor::HexRgb).toStdString();
//  opts.strokeWidth = getStrokeWidth();
  return opts;
}

/*
void ExportGcodeDialog::on_toolButtonFillColor_clicked()
{
  QColor color = QColorDialog::getColor(fillColor, this, tr("Select Fill Color"));
  if (color.isValid()) {
    updateFillColor(color);
  }
}

void ExportGcodeDialog::on_toolButtonFillColorReset_clicked()
{
  updateFillColor(QColor(Qt::white));
}

void ExportGcodeDialog::on_checkBoxEnableFill_toggled(bool checked)
{
  updateFillControlsEnabled();
}

void ExportGcodeDialog::on_toolButtonStrokeColor_clicked()
{
  QColor color = QColorDialog::getColor(strokeColor, this, tr("Select Stroke Color"));
  if (color.isValid()) {
    updateStrokeColor(color);
  }
}

void ExportGcodeDialog::on_toolButtonStrokeColorReset_clicked()
{
  updateStrokeColor(QColor(Qt::black));
}

void ExportGcodeDialog::on_checkBoxEnableStroke_toggled(bool checked)
{
  updateStrokeControlsEnabled();
}

void ExportGcodeDialog::on_toolButtonStrokeWidthReset_clicked()
{
  doubleSpinBoxStrokeWidth->setValue(defaultStrokeWidth);
}

void ExportGcodeDialog::on_pushButtonOk_clicked()
{
  accept();
}

void ExportGcodeDialog::on_pushButtonCancel_clicked()
{
  reject();
}

void ExportGcodeDialog::updateFillColor(const QColor& color)
{
  fillColor = color;
  QPalette pal = labelFillColor->palette();
  pal.setColor(QPalette::Window, color);
  labelFillColor->setAutoFillBackground(true);
  labelFillColor->setPalette(pal);
  labelFillColor->update();
}

void ExportGcodeDialog::updateFillControlsEnabled()
{
  bool enabled = checkBoxEnableFill->isChecked();
  labelFillColor->setEnabled(enabled);
  toolButtonFillColor->setEnabled(enabled);
  toolButtonFillColorReset->setEnabled(enabled);
}

void ExportGcodeDialog::updateStrokeColor(const QColor& color)
{
  strokeColor = color;
  QPalette pal = labelStrokeColor->palette();
  pal.setColor(QPalette::Window, color);
  labelStrokeColor->setAutoFillBackground(true);
  labelStrokeColor->setPalette(pal);
  labelStrokeColor->update();
}

void ExportGcodeDialog::updateStrokeControlsEnabled()
{
  bool enabled = checkBoxEnableStroke->isChecked();
  labelStrokeColor->setEnabled(enabled);
  toolButtonStrokeColor->setEnabled(enabled);
  toolButtonStrokeColorReset->setEnabled(enabled);
  labelStrokeWidth->setEnabled(enabled);
  doubleSpinBoxStrokeWidth->setEnabled(enabled);
  toolButtonStrokeWidthReset->setEnabled(enabled);
}
*/

#include "ExportSvgDialog.h"
#include <QColorDialog>
#include <QPushButton>

ExportSvgDialog::ExportSvgDialog()
{
  setupUi(this);
  fillColor = QColor(Qt::white);
  strokeColor = QColor(Qt::black);
  doubleSpinBoxStrokeWidth->setValue(defaultStrokeWidth);
  updateFillColor(fillColor);
  updateStrokeColor(strokeColor);
  updateFillControlsEnabled();
  updateStrokeControlsEnabled();

  connect(pushButtonOk, &QPushButton::clicked, this, &ExportSvgDialog::accept);
  connect(pushButtonCancel, &QPushButton::clicked, this, &ExportSvgDialog::reject);
}

int ExportSvgDialog::exec() { return QDialog::exec(); }

QColor ExportSvgDialog::getFillColor() const { return fillColor; }

bool ExportSvgDialog::isFillEnabled() const { return checkBoxEnableFill->isChecked(); }

QColor ExportSvgDialog::getStrokeColor() const { return strokeColor; }

bool ExportSvgDialog::isStrokeEnabled() const { return checkBoxEnableStroke->isChecked(); }

double ExportSvgDialog::getStrokeWidth() const { return doubleSpinBoxStrokeWidth->value(); }

ExportSvgOptions ExportSvgDialog::getOptions() const
{
  ExportSvgOptions opts;
  opts.fill = isFillEnabled();
  opts.fillColor = getFillColor().name(QColor::HexRgb).toStdString();
  opts.stroke = isStrokeEnabled();
  opts.strokeColor = getStrokeColor().name(QColor::HexRgb).toStdString();
  opts.strokeWidth = getStrokeWidth();
  return opts;
}

void ExportSvgDialog::on_toolButtonFillColor_clicked()
{
  QColor color = QColorDialog::getColor(fillColor, this, tr("Select Fill Color"));
  if (color.isValid()) {
    updateFillColor(color);
  }
}

void ExportSvgDialog::on_toolButtonFillColorReset_clicked() { updateFillColor(QColor(Qt::white)); }

void ExportSvgDialog::on_checkBoxEnableFill_toggled(bool checked) { updateFillControlsEnabled(); }

void ExportSvgDialog::on_toolButtonStrokeColor_clicked()
{
  QColor color = QColorDialog::getColor(strokeColor, this, tr("Select Stroke Color"));
  if (color.isValid()) {
    updateStrokeColor(color);
  }
}

void ExportSvgDialog::on_toolButtonStrokeColorReset_clicked() { updateStrokeColor(QColor(Qt::black)); }

void ExportSvgDialog::on_checkBoxEnableStroke_toggled(bool checked) { updateStrokeControlsEnabled(); }

void ExportSvgDialog::on_toolButtonStrokeWidthReset_clicked()
{
  doubleSpinBoxStrokeWidth->setValue(defaultStrokeWidth);
}

void ExportSvgDialog::updateFillColor(const QColor& color)
{
  fillColor = color;
  QPalette pal = labelFillColor->palette();
  pal.setColor(QPalette::Window, color);
  labelFillColor->setAutoFillBackground(true);
  labelFillColor->setPalette(pal);
  labelFillColor->update();
}

void ExportSvgDialog::updateFillControlsEnabled()
{
  bool enabled = checkBoxEnableFill->isChecked();
  labelFillColor->setEnabled(enabled);
  toolButtonFillColor->setEnabled(enabled);
  toolButtonFillColorReset->setEnabled(enabled);
}

void ExportSvgDialog::updateStrokeColor(const QColor& color)
{
  strokeColor = color;
  QPalette pal = labelStrokeColor->palette();
  pal.setColor(QPalette::Window, color);
  labelStrokeColor->setAutoFillBackground(true);
  labelStrokeColor->setPalette(pal);
  labelStrokeColor->update();
}

void ExportSvgDialog::updateStrokeControlsEnabled()
{
  bool enabled = checkBoxEnableStroke->isChecked();
  labelStrokeColor->setEnabled(enabled);
  toolButtonStrokeColor->setEnabled(enabled);
  toolButtonStrokeColorReset->setEnabled(enabled);
  labelStrokeWidth->setEnabled(enabled);
  doubleSpinBoxStrokeWidth->setEnabled(enabled);
  toolButtonStrokeWidthReset->setEnabled(enabled);
}

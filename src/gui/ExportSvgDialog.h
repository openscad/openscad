#pragma once

#include <memory>
#include <QDialog>
#include <QColor>

#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ExportSvgDialog.h"
#include "gui/InitConfigurator.h"
#include "io/export.h"

class ExportSvgDialog : public QDialog, public Ui::ExportSvgDialog, public InitConfigurator
{
  Q_OBJECT;

public:
  ExportSvgDialog();

  int exec() override;

  QColor getFillColor() const;
  bool isFillEnabled() const;
  QColor getStrokeColor() const;
  bool isStrokeEnabled() const;
  double getStrokeWidth() const;
  ExportSvgOptions getOptions() const;

private slots:
  void on_toolButtonFillColor_clicked();
  void on_toolButtonFillColorReset_clicked();
  void on_checkBoxEnableFill_toggled(bool checked);
  void on_toolButtonStrokeColor_clicked();
  void on_toolButtonStrokeColorReset_clicked();
  void on_checkBoxEnableStroke_toggled(bool checked);
  void on_toolButtonStrokeWidthReset_clicked();

private:
  void updateFillColor(const QColor& color);
  void updateFillControlsEnabled();
  void updateStrokeColor(const QColor& color);
  void updateStrokeControlsEnabled();

  QColor fillColor;
  QColor strokeColor;
  double defaultStrokeWidth = 0.35;
};
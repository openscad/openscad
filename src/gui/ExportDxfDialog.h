#pragma once

#include <QDialog>

#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ExportDxfDialog.h"
#include "gui/InitConfigurator.h"
#include "io/export.h"

class ExportDxfDialog : public QDialog, public Ui::ExportDxfDialog, public InitConfigurator
{
  Q_OBJECT;

public:
  ExportDxfDialog();

  int exec() override;
  void accept() override;

  ExportDxfOptions getOptions() const;
};

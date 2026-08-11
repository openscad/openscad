#include "ExportPngDialog.h"

#include "core/Settings.h"
#include "gui/SettingsWriter.h"

ExportPngDialog::ExportPngDialog()
{
  setupUi(this);
  checkBoxTransparentBackground->setChecked(Settings::Settings::exportPngTransparentBackground.value());
}

bool ExportPngDialog::isTransparentBackground() const
{
  return checkBoxTransparentBackground->isChecked();
}

void ExportPngDialog::on_pushButtonOk_clicked()
{
  // Remember the choice, so exporting a series of images doesn't mean re-ticking it every time.
  Settings::Settings::exportPngTransparentBackground.setValue(isTransparentBackground());
  Settings::Settings::visit(SettingsWriter());
  accept();
}

void ExportPngDialog::on_pushButtonCancel_clicked()
{
  reject();
}

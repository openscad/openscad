#include "ExportDxfDialog.h"
#include <QPushButton>
#include "core/Settings.h"
#include "gui/SettingsWriter.h"

using S = Settings::SettingsExportDxf;

ExportDxfDialog::ExportDxfDialog()
{
  setupUi(this);
  this->checkBoxAlwaysShowDialog->setChecked(S::exportDxfAlwaysShowDialog.value());

  // Populate combo box from settings entry (fills items + selects persisted value)
  initComboBox(comboBoxVersion, Settings::SettingsExportDxf::exportDxfVersion);

  connect(pushButtonOk, &QPushButton::clicked, this, &ExportDxfDialog::accept);
  connect(pushButtonCancel, &QPushButton::clicked, this, &ExportDxfDialog::reject);
}

int ExportDxfDialog::exec()
{
  bool showDialog = this->checkBoxAlwaysShowDialog->isChecked();
  if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
    showDialog = true;
  }

  const auto result = showDialog ? QDialog::exec() : QDialog::Accepted;

  if (result == QDialog::Accepted) {
    S::exportDxfAlwaysShowDialog.setValue(this->checkBoxAlwaysShowDialog->isChecked());
    Settings::Settings::visit(SettingsWriter());
  }

  return result;
  // return QDialog::exec();
}

void ExportDxfDialog::accept()
{
  // Persist the chosen version before closing so the next export starts
  // from the same value (GUI fromSettings() and command-line both benefit).
  const int idx = comboBoxVersion->currentIndex();
  const auto& items = Settings::SettingsExportDxf::exportDxfVersion.items();
  if (idx >= 0 && idx < static_cast<int>(items.size())) {
    Settings::SettingsExportDxf::exportDxfVersion.setValue(items[idx].value);
  }
  writeSettings();
  QDialog::accept();
}

ExportDxfOptions ExportDxfDialog::getOptions() const
{
  const int idx = comboBoxVersion->currentIndex();
  const auto& items = Settings::SettingsExportDxf::exportDxfVersion.items();

  DxfVersion version = DxfVersion::Legacy;
  if (idx >= 0 && idx < static_cast<int>(items.size())) {
    version = items[idx].value;
  }

  return ExportDxfOptions{.version = version};
}

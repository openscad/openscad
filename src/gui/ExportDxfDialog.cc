#include "ExportDxfDialog.h"
#include <QPushButton>
#include "core/Settings.h"

ExportDxfDialog::ExportDxfDialog()
{
  setupUi(this);

  // Populate combo box from settings entry (fills items + selects persisted value)
  initComboBox(comboBoxVersion, Settings::SettingsExportDxf::exportDxfVersion);

  connect(pushButtonOk, &QPushButton::clicked, this, &ExportDxfDialog::accept);
  connect(pushButtonCancel, &QPushButton::clicked, this, &ExportDxfDialog::reject);
}

int ExportDxfDialog::exec()
{
  return QDialog::exec();
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

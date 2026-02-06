#include "gui/OpenCSGWarningDialog.h"
#include <QString>
#include <QWidget>
#include "gui/Preferences.h"

OpenCSGWarningDialog::OpenCSGWarningDialog(QWidget *)
{
  setupUi(this);
}

void OpenCSGWarningDialog::on_showBox_toggled(bool checked)
{
  GlobalPreferences::inst()->openCSGWarningBox->setChecked(checked);
  GlobalPreferences::inst()->on_openCSGWarningBox_toggled(checked);
}

void OpenCSGWarningDialog::setText(const QString& text)
{
  this->warningText->setPlainText(text);
}

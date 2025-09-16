#include "gui/OpenCSGWarningDialog.h"
#include <QString>
#include <QWidget>
#include "gui/Preferences.h"

OpenCSGWarningDialog::OpenCSGWarningDialog(QWidget *)
{
  setupUi(this);

  connect(this->showBox, &QCheckBox::toggled, GlobalPreferences::inst()->openCSGWarningBox,
          &QCheckBox::setChecked);
  connect(this->showBox, &QCheckBox::toggled, GlobalPreferences::inst(),
          &Preferences::on_openCSGWarningBox_toggled);
}

void OpenCSGWarningDialog::setText(const QString& text) { this->warningText->setPlainText(text); }

#include "gui/OpenCSGWarningDialog.h"
#include <QString>
#include <QWidget>
#include "gui/Preferences.h"

OpenCSGWarningDialog::OpenCSGWarningDialog(QWidget *)
{
  setupUi(this);

  connect(this->showBox, SIGNAL(toggled(bool)),
          Preferences::inst()->openCSGWarningBox, SLOT(setChecked(bool)));
  connect(this->showBox, SIGNAL(toggled(bool)),
          Preferences::inst(), SLOT(on_openCSGWarningBox_toggled(bool)));
}

void OpenCSGWarningDialog::setText(const QString& text)
{
  this->warningText->setPlainText(text);
}


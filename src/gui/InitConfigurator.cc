
#include "gui/InitConfigurator.h"

#include <QListWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QString>
#include <QSettings>
#include <QLineEdit>
#include <QGroupBox>

#include "core/Settings.h"
#include "gui/SettingsWriter.h"

#include <string>

void InitConfigurator::writeSettings() { Settings::Settings::visit(SettingsWriter()); }

void InitConfigurator::initUpdateCheckBox(const BlockSignals<QCheckBox *>& checkBox,
                                          const Settings::SettingsEntryBool& entry)
{
  checkBox->setChecked(entry.value());
}

void InitConfigurator::initIntSpinBox(const BlockSignals<QSpinBox *>& spinBox,
                                      const Settings::SettingsEntryInt& entry)
{
  spinBox->setMinimum(entry.minimum());
  spinBox->setMaximum(entry.maximum());
}

void InitConfigurator::updateIntSpinBox(const BlockSignals<QSpinBox *>& spinBox,
                                        const Settings::SettingsEntryInt& entry)
{
  spinBox->setValue(entry.value());
}

void InitConfigurator::initUpdateDoubleSpinBox(QDoubleSpinBox *spinBox,
                                               const Settings::SettingsEntryDouble& entry)
{
  spinBox->blockSignals(true);
  spinBox->setSingleStep(entry.step());
  spinBox->setMinimum(entry.minimum());
  spinBox->setMaximum(entry.maximum());
  spinBox->setValue(entry.value());
  spinBox->blockSignals(false);
}

void InitConfigurator::initListBox(QListWidget *listBox,
                                   const Settings::SettingsEntryList<Settings::LocalAppParameter>& list)
{
  listBox->blockSignals(true);
  listBox->clear();
  for (const auto& listitem : list.value()) {
    if (listitem.type == Settings::LocalAppParameterType::string) {
      const auto item =
        createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::string),
                       QString::fromStdString(listitem.value));
      listBox->insertItem(listBox->count(), item);
    } else if (listitem.type == Settings::LocalAppParameterType::file) {
      const auto item =
        createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::file));
      listBox->insertItem(listBox->count(), item);
    } else if (listitem.type == Settings::LocalAppParameterType::dir) {
      const auto item =
        createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::dir));
      listBox->insertItem(listBox->count(), item);
    } else if (listitem.type == Settings::LocalAppParameterType::extension) {
      const auto item =
        createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::extension));
      listBox->insertItem(listBox->count(), item);
    } else if (listitem.type == Settings::LocalAppParameterType::source) {
      const auto item =
        createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::source));
      listBox->insertItem(listBox->count(), item);
    } else if (listitem.type == Settings::LocalAppParameterType::sourcedir) {
      const auto item =
        createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::sourcedir));
      listBox->insertItem(listBox->count(), item);
    }
  }
  listBox->selectionModel()->clearSelection();
  listBox->blockSignals(false);
}

void InitConfigurator::updateComboBox(const BlockSignals<QComboBox *>& comboBox,
                                      const std::string& value)
{
  int index = comboBox->findData(QString::fromStdString(value));
  if (index >= 0) {
    comboBox->setCurrentIndex(index);
  } else {
    comboBox->setCurrentIndex(0);
  }
}

void InitConfigurator::initMetaData(QCheckBox *checkBox, QLineEdit *lineEdit,
                                    Settings::SettingsEntryBool *settingsEntryFlag,
                                    Settings::SettingsEntryString& settingsEntry)
{
  lineEdit->setText(QString::fromStdString(settingsEntry.value()));
  if (checkBox && settingsEntryFlag) {
    checkBox->setChecked(settingsEntryFlag->value());
  }
}

void InitConfigurator::applyMetaData(const QCheckBox *checkBox, const QLineEdit *lineEdit,
                                     Settings::SettingsEntryBool *settingsEntryFlag,
                                     Settings::SettingsEntryString& settingsEntry)
{
  if (checkBox && settingsEntryFlag) {
    settingsEntryFlag->setValue(checkBox->isChecked());
  }
  const auto value = lineEdit->text().trimmed().toStdString();
  settingsEntry.setValue(value);
  return;
}

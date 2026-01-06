#pragma once

#include <QSettings>
#include <QObject>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QButtonGroup>
#include <optional>
#include <string>

#include "core/Settings.h"

template <class WidgetPtr>
class BlockSignals
{
public:
  BlockSignals(WidgetPtr w) : w(w) { w->blockSignals(true); }
  ~BlockSignals() { w->blockSignals(false); }
  WidgetPtr operator->() const { return w; }

  BlockSignals(const BlockSignals&) = delete;
  BlockSignals& operator=(BlockSignals const&) = delete;

private:
  WidgetPtr w;
};

class InitConfigurator
{
protected:
  void writeSettings();
  /** Set checkbox status from the settings value */
  void initUpdateCheckBox(const BlockSignals<QCheckBox *>& checkBox,
                          const Settings::SettingsEntryBool& entry);
  /** Initialize spinbox min/max values from the settings range values */
  void initIntSpinBox(const BlockSignals<QSpinBox *>& spinBox, const Settings::SettingsEntryInt& entry);
  /** Set spinbox value from the settings value */
  void updateIntSpinBox(const BlockSignals<QSpinBox *>& spinBox,
                        const Settings::SettingsEntryInt& entry);
  /** Set spinbox value and min/max/step from the settings value */
  void initUpdateDoubleSpinBox(QDoubleSpinBox *spinBox, const Settings::SettingsEntryDouble& entry);
  /** Initialize specialized list box */
  void initListBox(QListWidget *listBox,
                   const Settings::SettingsEntryList<Settings::LocalAppParameter>& list);
  /** Initialize combobox list values from the settings range values */
  template <typename enum_type>
  void initComboBox(QComboBox *comboBox, const Settings::SettingsEntryEnum<enum_type>& entry);
  /** Update combobox from current settings */
  template <typename enum_type>
  void updateComboBox(const BlockSignals<QComboBox *>& comboBox,
                      const Settings::SettingsEntryEnum<enum_type>& entry);
  /** Update combobox from current settings */
  void updateComboBox(const BlockSignals<QComboBox *>& comboBox, const std::string& value);
  /** Get enum value from combobox associated with a enum based settings entry */
  template <typename enum_type>
  std::optional<enum_type> getComboBoxValue(const BlockSignals<QComboBox *>& comboBox,
                                            const Settings::SettingsEntryEnum<enum_type>& entry)
  {
    if (comboBox->currentIndex() > 0) {
      const auto key = comboBox->currentData().toString().toStdString();
      for (const auto& e : entry.items()) {
        if (key == e.name) {
          return std::make_optional(e.value);
        }
      }
    }
    return std::nullopt;
  }
  /** Init a button group with an enum setting, this needs a custom property on the radio buttons */
  template <typename enum_type>
  void initButtonGroup(const BlockSignals<QButtonGroup *>& buttonGroup,
                       const Settings::SettingsEntryEnum<enum_type>& entry);
  /** Apply selected value from button to settings, this needs a custom property on the radio buttons */
  template <typename enum_type>
  void applyButtonGroup(const BlockSignals<QButtonGroup *>& buttonGroup,
                        Settings::SettingsEntryEnum<enum_type>& entry);

  void initMetaData(QCheckBox *, QLineEdit *, Settings::SettingsEntryBool *,
                    Settings::SettingsEntryString&);
  void applyMetaData(const QCheckBox *, const QLineEdit *, Settings::SettingsEntryBool *,
                     Settings::SettingsEntryString&);

  template <typename item_type>
  QListWidgetItem *createListItem(const item_type& itemType, const QString& text = "",
                                  bool editable = false)
  {
    const auto icon = QIcon::fromTheme(QString::fromStdString(itemType.icon()));
    std::string description = itemType.description();
    const auto itemText = description.empty() ? text : QString::fromStdString(description);
    const auto listItem = new QListWidgetItem(
      icon, itemText, nullptr, static_cast<int>(QListWidgetItem::UserType) + static_cast<int>(itemType));
    if (editable) {
      listItem->setFlags(listItem->flags() | Qt::ItemIsEditable);
    }
    return listItem;
  }
};

template <typename enum_type>
void InitConfigurator::initComboBox(QComboBox *comboBox,
                                    const Settings::SettingsEntryEnum<enum_type>& entry)
{
  comboBox->clear();
  for (const auto& item : entry.items()) {
    comboBox->addItem(QString::fromStdString(item.description), QString::fromStdString(item.name));
  }
  updateComboBox(comboBox, entry);
}

template <typename enum_type>
void InitConfigurator::updateComboBox(const BlockSignals<QComboBox *>& comboBox,
                                      const Settings::SettingsEntryEnum<enum_type>& entry)
{
  comboBox->setCurrentIndex(entry.index());
}

template <typename enum_type>
void InitConfigurator::initButtonGroup(const BlockSignals<QButtonGroup *>& buttonGroup,
                                       const Settings::SettingsEntryEnum<enum_type>& entry)
{
  for (const auto button : buttonGroup->buttons()) {
    const auto settingsValue = button->property(Settings::PROPERTY_NAME).toString().toStdString();
    if (settingsValue == entry.item().name) {
      button->setChecked(true);
    }
  }
}

template <typename enum_type>
void InitConfigurator::applyButtonGroup(const BlockSignals<QButtonGroup *>& buttonGroup,
                                        Settings::SettingsEntryEnum<enum_type>& entry)
{
  const auto button = buttonGroup->checkedButton();
  if (button) {
    const auto settingsValue = button->property(Settings::PROPERTY_NAME).toString().toStdString();
    entry.setValue(entry.decode(settingsValue));
  }
}

#pragma once
#include <QSettings>
#include <QObject>
#include "gui/Settings.h"
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

#include <string>

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
  /** Set checkbox status from the settings value */
  void initUpdateCheckBox(const BlockSignals<QCheckBox *>& checkBox, const Settings::SettingsEntryBool& entry);
  /** Initialize spinbox min/max values from the settings range values */
  void initIntSpinBox(const BlockSignals<QSpinBox *>& spinBox, const Settings::SettingsEntryInt& entry);
  /** Set spinbox value from the settings value */
  void updateIntSpinBox(const BlockSignals<QSpinBox *>& spinBox, const Settings::SettingsEntryInt& entry);
  /** Set spinbox value and min/max/step from the settings value */
  void initUpdateDoubleSpinBox(QDoubleSpinBox *spinBox, const Settings::SettingsEntryDouble& entry);
  /** Initialize combobox list values from the settings range values */
  void initComboBox(QComboBox *comboBox, const Settings::SettingsEntryEnum& entry);
  /** Update combobox from current settings */
  void updateComboBox(const BlockSignals<QComboBox *>& comboBox, const Settings::SettingsEntryEnum& entry);
  /** Update combobox from current settings */
  void updateComboBox(const BlockSignals<QComboBox *>& comboBox, const std::string& value);
};

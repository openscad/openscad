#pragma once

#include "gui/qtgettext.h"
#include <QComboBox>
#include <QWidget>
#include "ui_MouseConfigWidget.h"
#include "core/Settings.h"

class MouseConfigWidget : public QWidget, public Ui::Mouse
{
  Q_OBJECT

public:
  MouseConfigWidget(QWidget *parent = nullptr);
  void updateMouseState(int, bool) const;
  void init();

public slots:
  void on_comboBoxPreset_activated(int val);
  void on_comboBoxLeftClick_activated(int val);
  void on_comboBoxMiddleClick_activated(int val);
  void on_comboBoxRightClick_activated(int val);
  void on_comboBoxShiftLeftClick_activated(int val);
  void on_comboBoxShiftMiddleClick_activated(int val);
  void on_comboBoxShiftRightClick_activated(int val);
  void on_comboBoxCtrlLeftClick_activated(int val);
  void on_comboBoxCtrlMiddleClick_activated(int val);
  void on_comboBoxCtrlRightClick_activated(int val);

signals:
  void inputMappingChanged() const;

private:
  /** Initialize combobox list values from the settings range values */
  void initActionComboBox(QComboBox *comboBox, const Settings::SettingsEntryString& entry);
  /** Update combobox from current settings */
  void updateComboBox(QComboBox *comboBox, const Settings::SettingsEntryString& entry);
  /** Set value from combobox to settings */
  void applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntryString& entry);
  void writeSettings();

  const QString EmptyString = QString("");
  const QString ActiveStyleString = QString("font-weight: bold; color: red");
  const QString DisabledStyleString = QString("color: gray");

  bool initialized = false;
};

#pragma once

#include "gui/qtgettext.h"
#include <QComboBox>
#include <QWidget>
#include "ui_ButtonConfigWidget.h"
#include "gui/Settings.h"

class ButtonConfigWidget : public QWidget, public Ui::Button
{
  Q_OBJECT

public:
  ButtonConfigWidget(QWidget *parent = nullptr);
  void updateButtonState(int, bool) const;
  void init();
  void updateStates();

public slots:
  void on_comboBoxButton0_activated(int val);
  void on_comboBoxButton1_activated(int val);
  void on_comboBoxButton2_activated(int val);
  void on_comboBoxButton3_activated(int val);
  void on_comboBoxButton4_activated(int val);
  void on_comboBoxButton5_activated(int val);
  void on_comboBoxButton6_activated(int val);
  void on_comboBoxButton7_activated(int val);
  void on_comboBoxButton8_activated(int val);
  void on_comboBoxButton9_activated(int val);
  void on_comboBoxButton10_activated(int val);
  void on_comboBoxButton11_activated(int val);
  void on_comboBoxButton12_activated(int val);
  void on_comboBoxButton13_activated(int val);
  void on_comboBoxButton14_activated(int val);
  void on_comboBoxButton15_activated(int val);
  void on_comboBoxButton16_activated(int val);
  void on_comboBoxButton17_activated(int val);
  void on_comboBoxButton18_activated(int val);
  void on_comboBoxButton19_activated(int val);
  void on_comboBoxButton20_activated(int val);
  void on_comboBoxButton21_activated(int val);
  void on_comboBoxButton22_activated(int val);
  void on_comboBoxButton23_activated(int val);

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

#pragma once

#include <QMainWindow>
#include <QSettings>

#include "qtgettext.h"

#include "ui_ButtonConfigWidget.h"
#include "settings.h"

class ButtonConfigWidget: public QWidget, public Ui::Button
{
	Q_OBJECT

public:
	ButtonConfigWidget(QWidget *parent = 0);
	virtual ~ButtonConfigWidget();
	void updateButtonState(int,bool) const;
	void init();

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

signals:
        void inputMappingChanged() const;
  
private:
	/** Initialize combobox list values from the settings range values */
	void initComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry);
	/** Initialize spinbox min/max values from the settings range values */
	//void initSpinBox(QSpinBox *spinBox, const Settings::SettingsEntry& entry);
	//void initDoubleSpinBox(QDoubleSpinBox *spinBox, const Settings::SettingsEntry& entry);
	/** Update combobox from current settings */
	void updateComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry);
	/** Set value from combobox to settings */
	void applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntry& entry);
	void writeSettings();

	const QString EmptyString= QString("");
	const QString ActiveStyleString= QString("font-weight: bold; color: red");
};

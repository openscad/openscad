#pragma once

#include <QSettings>

#include "qtgettext.h"

#include "settings.h"
#include "ui_AxisConfigWidget.h"

class AxisConfigWidget : public QWidget, public Ui::Axis
{
	Q_OBJECT

public:
	//~AxisConfigWidget();
	void updateButtonState(int,bool) const;
	void AxesChanged(int nr, double val) const;
	void init();

public slots:
	// Input Driver
        void on_AxisTrim();
        void on_AxisTrimReset();
        void on_comboBoxTranslationX_activated(int val);
        void on_comboBoxTranslationY_activated(int val);
        void on_comboBoxTranslationZ_activated(int val);
        void on_comboBoxTranslationXVPRel_activated(int val);
        void on_comboBoxTranslationYVPRel_activated(int val);
        void on_comboBoxTranslationZVPRel_activated(int val);
        void on_comboBoxRotationX_activated(int val);
        void on_comboBoxRotationY_activated(int val);
        void on_comboBoxRotationZ_activated(int val);
        void on_comboBoxZoom_activated(int val);
        void on_comboBoxZoom2_activated(int val);


	void on_doubleSpinBoxRotateGain_valueChanged(double val);
	void on_doubleSpinBoxTranslationGain_valueChanged(double val);
	void on_doubleSpinBoxTranslationVPRelGain_valueChanged(double val);
	void on_doubleSpinBoxZoomGain_valueChanged(double val);

	void on_doubleSpinBoxDeadzone0_valueChanged(double);
	void on_doubleSpinBoxDeadzone1_valueChanged(double);
	void on_doubleSpinBoxDeadzone2_valueChanged(double);
	void on_doubleSpinBoxDeadzone3_valueChanged(double);
	void on_doubleSpinBoxDeadzone4_valueChanged(double);
	void on_doubleSpinBoxDeadzone5_valueChanged(double);
	void on_doubleSpinBoxDeadzone6_valueChanged(double);
	void on_doubleSpinBoxDeadzone7_valueChanged(double);
	void on_doubleSpinBoxDeadzone8_valueChanged(double);

	void on_doubleSpinBoxTrim0_valueChanged(double);
	void on_doubleSpinBoxTrim1_valueChanged(double);
	void on_doubleSpinBoxTrim2_valueChanged(double);
	void on_doubleSpinBoxTrim3_valueChanged(double);
	void on_doubleSpinBoxTrim4_valueChanged(double);
	void on_doubleSpinBoxTrim5_valueChanged(double);
	void on_doubleSpinBoxTrim6_valueChanged(double);
	void on_doubleSpinBoxTrim7_valueChanged(double);
	void on_doubleSpinBoxTrim8_valueChanged(double);

signals:
        void inputMappingChanged() const;
        void inputCalibrationChanged() const;
        void inputGainChanged() const;

private:
	/** Initialize combobox list values from the settings range values */
	void initComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry);
	/** Initialize spinbox min/max values from the settings range values */
	void initSpinBox(QSpinBox *spinBox, const Settings::SettingsEntry& entry);
	void initDoubleSpinBox(QDoubleSpinBox *spinBox, const Settings::SettingsEntry& entry);
	/** Update combobox from current settings */
	void updateComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry);
	/** Set value from combobox to settings */
	void applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntry& entry);
	void writeSettings();
};

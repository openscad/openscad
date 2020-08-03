#pragma once
#include <QSettings>
#include<QObject>
#include "settings.h"
#include "qtgettext.h"
#include<QComboBox>
#include<QSpinBox>
#include<QCheckBox>

template <class WidgetPtr>
class BlockSignals
{
public:
	BlockSignals(WidgetPtr w) : w(w) { w->blockSignals(true); }
	~BlockSignals() { w->blockSignals(false); }
	WidgetPtr operator->() const { return w; }

	BlockSignals(const BlockSignals &) = delete;
	BlockSignals &operator=(BlockSignals const &) = delete;

private:
	WidgetPtr w;
};

class InitConfigurator
{
protected:
	InitConfigurator();

	/** Initialize checkbox from the settings value */
	void initCheckBox(const BlockSignals<QCheckBox *> &checkBox,const Settings::SettingsEntry &entry);
	/** Initialize combobox list values from the settings range values */
	void initComboBox(QComboBox *comboBox, const Settings::SettingsEntry &entry);
	/** Initialize spinbox min/max values from the settings range values */
	void initSpinBoxRange(const BlockSignals<QSpinBox *> &spinBox,const Settings::SettingsEntry &entry);
	/** Initialize spinbox double value from the settings value */
	void initSpinBoxDouble(const BlockSignals<QSpinBox *> &spinBox, const Settings::SettingsEntry &entry);
	/** Update combobox from current settings */
	void updateComboBox(const BlockSignals<QComboBox *> &comboBox,const Settings::SettingsEntry &entry);
	/** used in AxisConfigWidget class **/
	void initDoubleSpinBox(QDoubleSpinBox *spinBox,const Settings::SettingsEntry &entry);

};
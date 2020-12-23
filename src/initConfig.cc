#include "initConfig.h"
#include <QSettings>
#include "settings.h"

InitConfigurator::InitConfigurator() {}

void InitConfigurator::initCheckBox(const BlockSignals<QCheckBox *> &checkBox,const Settings::SettingsEntry &entry)
{
	const Settings::Settings *s = Settings::Settings::inst();
	checkBox->setChecked(s->get(entry).toBool());
}

void InitConfigurator::initComboBox(QComboBox *comboBox, const Settings::SettingsEntry &entry)
{
	comboBox->clear();
	// Range is a vector of 2D vectors: [[name, value], ...]
	for (const auto &v : entry.range().toVector()) {
		QString val = QString::fromStdString(v[0].toString());
		QString qtext = QString::fromStdString(gettext(v[1].toString().c_str()));
		comboBox->addItem(qtext, val);
	}
}

void InitConfigurator::initSpinBoxRange(const BlockSignals<QSpinBox *> &spinBox,const Settings::SettingsEntry &entry)
{
	const RangeType &range = entry.range().toRange();
	spinBox->setMinimum(range.begin_value());
	spinBox->setMaximum(range.end_value());
}

void InitConfigurator::initSpinBoxDouble(const BlockSignals<QSpinBox *> &spinBox,const Settings::SettingsEntry &entry)
{
	const Settings::Settings *s = Settings::Settings::inst();
	spinBox->setValue(s->get(entry).toDouble());
}

void InitConfigurator::updateComboBox(const BlockSignals<QComboBox *> &comboBox,const Settings::SettingsEntry &entry)
{
	Settings::Settings *s = Settings::Settings::inst();

	const Value &value = s->get(entry);
	QString text = QString::fromStdString(value.toString());
	int idx = comboBox->findData(text);
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	}
	else {
		const Value &defaultValue = entry.defaultValue();
		QString defaultText = QString::fromStdString(defaultValue.toString());
		int defIdx = comboBox->findData(defaultText);
		if (defIdx >= 0) {
			comboBox->setCurrentIndex(defIdx);
		}
		else {
			comboBox->setCurrentIndex(0);
		}
	}
}

void InitConfigurator::initDoubleSpinBox(QDoubleSpinBox *spinBox,const Settings::SettingsEntry &entry)
{
	Settings::Settings *s = Settings::Settings::inst();

	const RangeType &range = entry.range().toRange();
	spinBox->blockSignals(true);
	spinBox->setMinimum(range.begin_value());
	spinBox->setSingleStep(range.step_value());
	spinBox->setMaximum(range.end_value());
	spinBox->setValue((double)s->get(entry).toDouble());
	spinBox->blockSignals(false);
}


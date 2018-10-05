#include <iostream>
#include "QSettingsCached.h"

#include "Dock.h"

Dock::Dock(QWidget *parent) : QDockWidget(parent), action(nullptr), updateSettings(true)
{
}

Dock::~Dock()
{
}

void Dock::disableSettingsUpdate()
{
	updateSettings = false;
}

void Dock::setVisible(bool visible)
{
	if (updateSettings) {
		QSettingsCached settings;
		settings.setValue(configKey, !visible);
	}
	if (action != nullptr) {
		action->setChecked(!visible);
	}
	QDockWidget::setVisible(visible);
}

void Dock::setConfigKey(const QString configKey)
{
	this->configKey = configKey;
}

void Dock::setAction(QAction *action)
{
	this->action = action;
}

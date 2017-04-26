#include <iostream>
#include <QSettings>

#include "Dock.h"

Dock::Dock(QWidget *parent) : QDockWidget(parent), action(nullptr)
{
}

Dock::~Dock()
{
}

void Dock::setVisible(bool visible)
{
	QSettings settings;
	settings.setValue(configKey, !visible);
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

#include <iostream>
#include <QSettings>

#include "Dock.h"

Dock::Dock(QWidget *parent) : QDockWidget(parent), action(NULL)
{
}

Dock::~Dock()
{
}

void Dock::setVisible(bool visible)
{
    QSettings settings;
    settings.setValue(configKey, !visible);
    if (action != NULL) {
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
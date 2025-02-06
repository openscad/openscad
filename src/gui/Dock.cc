#include "gui/Dock.h"

#include <QDockWidget>
#include <QWidget>
#include "gui/QSettingsCached.h"


Dock::Dock(QWidget *parent) : QDockWidget(parent)
{
    connect(this, &QDockWidget::topLevelChanged, this, &Dock::onTopLevelStatusChanged);
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

void Dock::setConfigKey(const QString& configKey)
{
  this->configKey = configKey;
}

void Dock::setAction(QAction *action)
{
  this->action = action;
}

void Dock::updateTitle(){
    QString title(name);
    if(isTopLevel() && !namesuffix.isEmpty()){
        title += " (" + namesuffix + ")";
    }
    setWindowTitle(title);
}

void Dock::setName(const QString& name_) {
    name = name_;
    updateTitle();
}

QString Dock::getName() const {
    return name;
}

void Dock::setNameSuffix(const QString& namesuffix_) {
    namesuffix = namesuffix_;
    updateTitle();
}

void Dock::onTopLevelStatusChanged(bool isTopLevel)
{
    // update the title of the window so it contains the title suffix (in general filename)
    // also update the flags and visibility to provide interactive feedback on the user action
    // while it is moving the dock in topLevel=true state.
    Qt::WindowFlags flags = (windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
    if (isTopLevel){
      setWindowFlags(flags);
      show();
    }
    updateTitle();
}

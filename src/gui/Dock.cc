#include "gui/Dock.h"

#include <QDockWidget>
#include <QWidget>
#include "gui/QSettingsCached.h"

Dock::Dock(QWidget *parent) : QDockWidget(parent)
{
  connect(this, &QDockWidget::topLevelChanged, this, &Dock::onTopLevelStatusChanged);
  connect(this, &QDockWidget::visibilityChanged, this, &Dock::onVisibilityChanged);

  dockTitleWidget = new QWidget();
}

Dock::~Dock() { delete dockTitleWidget; }

void Dock::disableSettingsUpdate() { updateSettings = false; }

void Dock::onVisibilityChanged(bool isDockVisible)
{
  if (updateSettings) {
    QSettingsCached settings;
    settings.setValue(configKey, !isVisible());
  }
}

void Dock::setTitleBarVisibility(bool isVisible)
{
  setTitleBarWidget(isVisible ? dockTitleWidget : nullptr);
}

void Dock::setConfigKey(const QString& configKey) { this->configKey = configKey; }

void Dock::updateTitle()
{
  QString title(name);
  if (isFloating() && !namesuffix.isEmpty()) {
    title += " (" + namesuffix + ")";
  }
  setWindowTitle(title);
}

void Dock::setName(const QString& name_)
{
  name = name_;
  updateTitle();
}

QString Dock::getName() const { return name; }

void Dock::setNameSuffix(const QString& namesuffix_)
{
  namesuffix = namesuffix_;
  updateTitle();
}

void Dock::onTopLevelStatusChanged(bool isTopLevel)
{
  // update the title of the window so it contains the title suffix (in general filename)
  // also update the flags and visibility to provide interactive feedback on the user action
  // while it is moving the dock in topLevel=true state. The purpose of such setting
  // on Qt::Window flag is to allow the dock to be floating behind the main window,
  // something which isn't supported for regular QDockWidgets.
  Qt::WindowFlags flags = (windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (isTopLevel) {
    setWindowFlags(flags);
    show();
  }
  updateTitle();
}

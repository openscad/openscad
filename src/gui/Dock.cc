#include "gui/Dock.h"

#include <QDockWidget>
#include <QRegularExpression>
#include <QWidget>

Dock::Dock(QWidget *parent) : QDockWidget(parent)
{
  // Showing a dock here would also show closed floating docks during restoreState().
  connect(this, &QDockWidget::topLevelChanged, this, &Dock::updateTitle);

  dockTitleWidget = new QWidget();
}

Dock::~Dock()
{
  delete dockTitleWidget;
}

// When moving docked panes around is allowed (reorderMode/reorderWindows), enable a title bar.
// When not allowed, disable the title bar.  Note that nullptr yields a default toolbar, while
// docTitleWidget is an empty (and so invisible) widget.
void Dock::setTitleBarVisibility(bool isVisible)
{
  setTitleBarWidget(isVisible ? nullptr : dockTitleWidget);
}

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
  // On Linux and Qt6.10 this is not needed, but older Qt versions
  // show the & mnemonic marker in the dock title. Allow keeping
  // single & characters not directly followed by a letter.
  name.replace(QRegularExpression("&([a-zA-Z])"), "\\1");
  updateTitle();
}

QString Dock::getName() const
{
  return name;
}

void Dock::setNameSuffix(const QString& namesuffix_)
{
  namesuffix = namesuffix_;
  updateTitle();
}

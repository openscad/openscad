#include "gui/Dock.h"

#include <QDockWidget>
#include <QRegularExpression>
#include <QWidget>

namespace {

QtMessageHandler originalHandler = nullptr;

void silentMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  if (type == QtWarningMsg && msg.contains("Already setting window visible")) {
    return;
  }
  if (originalHandler) {
    originalHandler(type, context, msg);
  }
}

class ScopedMessageSilencer
{
public:
  ScopedMessageSilencer() { originalHandler = qInstallMessageHandler(silentMessageOutput); }
  ~ScopedMessageSilencer() { qInstallMessageHandler(originalHandler); }
};

}  // namespace

Dock::Dock(QWidget *parent) : QDockWidget(parent)
{
  connect(this, &QDockWidget::topLevelChanged, this, &Dock::onTopLevelStatusChanged);

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

void Dock::onTopLevelStatusChanged(bool isTopLevel)
{
  // CAUTION:  A previous incarnation of this set the window type of an undocked window to Qt::Window
  // (rather than its default Qt::Tool) in an attempt to enable undocked windows to float behind the
  // main window.  That didn't work (or at least not on all platforms), and caused #6667, breaking
  // shortcuts when undocked.  See #6765 for more discussion.

  // update the title of the window so it contains the title suffix (in general filename)
  // also update the visibility to provide interactive feedback on the user action
  // while it is moving the dock in topLevel=true state.
  if (isTopLevel) {
    // show() emits an innocuous "Already setting window visible!" warning on macOS
    ScopedMessageSilencer silencer;
    show();
  }
  updateTitle();
}

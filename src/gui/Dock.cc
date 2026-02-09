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

void Dock::setTitleBarVisibility(bool isVisible)
{
  setTitleBarWidget(isVisible ? dockTitleWidget : nullptr);
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
  // update the title of the window so it contains the title suffix (in general filename)
  // also update the flags and visibility to provide interactive feedback on the user action
  // while it is moving the dock in topLevel=true state. The purpose of such setting
  // on Qt::Window flag is to allow the dock to be floating behind the main window,
  // something which isn't supported for regular QDockWidgets.
  Qt::WindowFlags flags = (windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (isTopLevel) {
    setWindowFlags(flags);
    // show() rmits an innocuous "Already setting window visible!" warning on macOS
    ScopedMessageSilencer silencer;
    show();
  }
  updateTitle();
}

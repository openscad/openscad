#include "gui/OpenSCADApp.h"

#include "gui/MainWindow.h"
#include "gui/Preferences.h"
#ifdef Q_OS_MACOS
#include "gui/EventFilter.h"
#endif

#include "geometry/GeometryCache.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALCache.h"
#endif
#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QProgressDialog>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <boost/foreach.hpp>
#include <QStyleHints>
#include <cassert>
#include <exception>

#if defined(Q_OS_LINUX) && defined(ENABLE_DBUS)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#endif

#include "glview/RenderSettings.h"

OpenSCADApp::OpenSCADApp(int& argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACOS
  this->installEventFilter(new SCADEventFilter(this));
#endif

  // Note: It may be tempting to add more initialization code here, but keep in mind that this is run as
  // part of QApplication initialization, so it's usually better to that in the main gui() function after
  // the OpenSCADApp instance is created.
}

OpenSCADApp::~OpenSCADApp()
{
  delete this->fontCacheDialog;
}

#include <QMessageBox>

bool OpenSCADApp::notify(QObject *object, QEvent *event)
{
  QString msg;
  try {
    return QApplication::notify(object, event);
  } catch (const std::exception& e) {
    msg = e.what();
  } catch (...) {
    msg = _("Unknown error");
  }
  // This happens when an uncaught exception is thrown in a Qt event handler
  QMessageBox::critical(
    nullptr, QString(_("Critical Error")),
    QString(_("A critical error was caught. The application may have become unstable:\n%1"))
      .arg(QString(msg)));
  return false;
}

/*!
   Requests to open a file from an external event, e.g. by double-clicking a filename.
 */
void OpenSCADApp::requestOpenFile(const QString& filename)
{
  for (auto win : this->windowManager.getWindows()) {
    // if we have an empty open window, use that one
    if (win->isEmpty()) {
      win->tabManager->createTab(filename);
      return;
    }
  }

  // ..otherwise, create a new one
  new MainWindow(QStringList(filename));
}

void OpenSCADApp::showFontCacheDialog()
{
  if (!this->fontCacheDialog) this->fontCacheDialog = new QProgressDialog();
  this->fontCacheDialog->setLabelText(
    _("Fontconfig needs to update its font cache.\nThis can take up to a couple of minutes."));
  this->fontCacheDialog->setMinimum(0);
  this->fontCacheDialog->setMaximum(0);
  this->fontCacheDialog->setCancelButton(nullptr);
  this->fontCacheDialog->exec();
}

void OpenSCADApp::hideFontCacheDialog()
{
  assert(this->fontCacheDialog);
  this->fontCacheDialog->reset();
}

void OpenSCADApp::setRenderBackend3D(RenderBackend3D backend)
{
  RenderSettings::inst()->backend3D = backend;
  CGALCache::instance()->clear();
  GeometryCache::instance()->clear();
}

static bool isSystemDarkTheme()
{
#if defined(Q_OS_LINUX) && defined(ENABLE_DBUS)
  // On Linux, neither Qt's colorScheme() API (Qt 6.5+) nor the palette
  // heuristic are reliable: GNOME's Qt platform plugin can read the
  // gtk-theme name (e.g. "Adwaita-dark") instead of the actual dark-style
  // preference, returning "dark" even when the desktop is set to light.
  //
  // The XDG Desktop Portal is the authoritative source — it reflects
  // GNOME's "Dark Style" toggle and works on KDE Plasma too.
  // Return value: 0 = no preference (light), 1 = prefer dark, 2 = prefer light.
  QDBusMessage msg =
    QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
                                   "org.freedesktop.portal.Settings", "Read");
  msg << "org.freedesktop.appearance" << "color-scheme";
  QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 250);
  if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
    const QVariant outer = reply.arguments().first();
    const QVariant inner = outer.value<QDBusVariant>().variant();
    const uint value = inner.value<QDBusVariant>().variant().toUInt();
    return value == 1;
  }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  // On non-Linux (Windows, macOS), Qt's colorScheme() API works correctly.
  const auto scheme = QGuiApplication::styleHints()->colorScheme();
  if (scheme == Qt::ColorScheme::Dark) return true;
  if (scheme == Qt::ColorScheme::Light) return false;
  return false;
#else
  // Fallback: palette-based heuristic (works on Windows and macOS where
  // the platform theme correctly reflects the system setting).
  const QPalette& pal = scadApp->palette();
  const auto& text = pal.color(QPalette::WindowText);
  const auto& window = pal.color(QPalette::Window);
  return text.lightness() > window.lightness();
#endif
}

void OpenSCADApp::setGuiTheme(const QString& preference)
{
  const bool useDark = [&preference]() {
    if (preference == "dark") return true;
    if (preference == "light") return false;
    // "auto": follow OS; if unknown, use light
    return isSystemDarkTheme();
  }();

  QStyle *fusion = QStyleFactory::create("Fusion");

  // Use Fusion style for all themes.  On Linux, platform theme plugins
  // poison even Fusion's standardPalette() with system colors, so we must
  // always set explicit palette colors.  On Windows/macOS the native style
  // offers little benefit when we override the palette anyway.
  if (fusion) scadApp->setStyle(fusion);

  if (useDark) {
    themePalette = QPalette();
    themePalette.setColor(QPalette::Window, QColor(53, 53, 53));
    themePalette.setColor(QPalette::WindowText, Qt::white);
    themePalette.setColor(QPalette::Base, QColor(25, 25, 25));
    themePalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    themePalette.setColor(QPalette::ToolTipBase, Qt::white);
    themePalette.setColor(QPalette::ToolTipText, Qt::white);
    themePalette.setColor(QPalette::Text, Qt::white);
    themePalette.setColor(QPalette::Button, QColor(53, 53, 53));
    themePalette.setColor(QPalette::ButtonText, Qt::white);
    themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    themePalette.setColor(QPalette::HighlightedText, Qt::black);
  } else {
    themePalette = QPalette();
    themePalette.setColor(QPalette::Window, QColor(239, 239, 239));
    themePalette.setColor(QPalette::WindowText, Qt::black);
    themePalette.setColor(QPalette::Base, Qt::white);
    themePalette.setColor(QPalette::AlternateBase, QColor(239, 239, 239));
    themePalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
    themePalette.setColor(QPalette::ToolTipText, Qt::black);
    themePalette.setColor(QPalette::Text, Qt::black);
    themePalette.setColor(QPalette::Button, QColor(239, 239, 239));
    themePalette.setColor(QPalette::ButtonText, Qt::black);
    themePalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    themePalette.setColor(QPalette::HighlightedText, Qt::white);
    themePalette.setColor(QPalette::Link, QColor(42, 130, 218));
    themePalette.setColor(QPalette::LinkVisited, QColor(100, 74, 155));
  }
  scadApp->setPalette(themePalette);

  QIcon::setThemeName(useDark ? "chokusen-dark" : "chokusen");

  const auto family = GlobalPreferences::inst()->getValue("advanced/applicationFontFamily").toString();
  const auto size = GlobalPreferences::inst()->getValue("advanced/applicationFontSize").toUInt();
  setApplicationFont(family, size);
}

void OpenSCADApp::setApplicationFont(const QString& family, uint size)
{
  // A global stylesheet with the * selector is the only reliable way to
  // force font updates on all existing widgets (dock title bars, status
  // bar, etc.).  However, setStyleSheet() triggers Qt platform-theme
  // plugins to re-apply the system palette — so we re-apply our explicit
  // theme palette immediately afterwards.
  const auto stylesheet = QString(R"(
    * {
        font-family: '%1';
        font-size: %2pt;
    }
    QStatusBar::item {
        border: 0px solid black;
    }
  )");
  scadApp->setStyleSheet(stylesheet.arg(family, QString::number(size)));
  scadApp->setPalette(themePalette);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// https://doc.qt.io/qt-6/qtcore-changes-qt6.html#other-classes
//
//     In Qt 5, QCoreApplication::quit() was equivalent to calling QCoreApplication::exit().
//     This just exited the main event loop.
//
//     In Qt 6, the method will instead try to close all top-level windows by posting a close
//     event. The windows are free to cancel the shutdown process by ignoring the event.
//
// For Qt5, simulate the Qt6 behavior.
void OpenSCADApp::quit()
{
  for (MainWindow *mw : scadApp->windowManager.getWindows()) {
    if (!mw->close()) {
      return;
    }
  }
  QApplication::quit();
}
#endif

#include "gui/OpenSCADApp.h"

#include "gui/MainWindow.h"
#include "gui/Preferences.h"
#include "gui/QSettingsCached.h"
#ifdef Q_OS_MACOS
#include "gui/EventFilter.h"
#endif

#include "geometry/GeometryCache.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALCache.h"
#endif
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QObject>
#include <QPalette>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QStyleHints>
#include <cassert>
#include <exception>
#include <QProgressDialog>
#include "glview/RenderSettings.h"

OpenSCADApp::OpenSCADApp(int& argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACOS
  this->installEventFilter(new SCADEventFilter(this));
#endif

  // Remember platform default style so we can restore it for light/native theme
  platformStyleName = style()->objectName();

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  const auto scheme = QGuiApplication::styleHints()->colorScheme();
  if (scheme == Qt::ColorScheme::Dark) return true;
  if (scheme == Qt::ColorScheme::Light) return false;
  // Unknown: fall back to light
  return false;
#else
  // Qt5: no colorScheme() API. Use palette heuristic from the application's
  // current palette (reflects platform theme on Linux; on Windows/macOS may
  // often be light). Fallback to light if unclear.
  const QPalette &pal = scadApp->palette();
  const auto &text = pal.color(QPalette::WindowText);
  const auto &window = pal.color(QPalette::Window);
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

  if (useDark) {
    QStyle *fusion = QStyleFactory::create("Fusion");
    if (fusion) scadApp->setStyle(fusion);
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    scadApp->setPalette(darkPalette);
  } else {
    // Use platform default style for a native look (Windows, macOS, Linux DE)
    QStyle *native = QStyleFactory::create(scadApp->platformStyleName);
    if (!native) native = QStyleFactory::create("Fusion");
    if (native) scadApp->setStyle(native);
    scadApp->setPalette(scadApp->style()->standardPalette());
  }

  QIcon::setThemeName(useDark ? "chokusen-dark" : "chokusen");

  // Re-apply application font (stylesheet is replaced when changing style/palette)
  const auto family = GlobalPreferences::inst()->getValue("advanced/applicationFontFamily").toString();
  const auto size = GlobalPreferences::inst()->getValue("advanced/applicationFontSize").toUInt();
  setApplicationFont(family, size);
}

void OpenSCADApp::setApplicationFont(const QString& family, uint size)
{
  // Trigger style sheet refresh to update the application font
  // (hopefully) everywhere. Also remove ugly frames in the QStatusBar
  // when using additional widgets
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

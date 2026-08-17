#pragma once

#include <QObject>

#include "gui/MainWindow.h"

// Tests that assert on the compute worker itself — its process id, its respawn
// behaviour, or that dispatch returned before any parsing happened — are only
// meaningful with process isolation on. The suite runs isolated by default and
// legacy when OPENSCAD_GUI_TEST_LEGACY is set; these skip in the latter.
#define SKIP_WITHOUT_PROCESS_ISOLATION()                                        \
  do {                                                                          \
    if (!MainWindow::isProcessIsolation()) QSKIP("requires process isolation"); \
  } while (false)

class UXTest : public QObject
{
  Q_OBJECT;

public:
  void setWindow(MainWindow *window);

protected:
  void restoreWindowInitialState();

  MainWindow *window;
};

#include "WindowManager.h"
#include "MainWindow.h"

void WindowManager::add(MainWindow *mainwin)
{
  this->windows.insert(mainwin);
}

void WindowManager::remove(MainWindow *mainwin)
{
  this->windows.remove(mainwin);
}

const QSet<MainWindow *>& WindowManager::getWindows() const
{
  return this->windows;
}

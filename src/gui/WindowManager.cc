#include "gui/WindowManager.h"

#include <QSet>

#include "gui/MainWindow.h"

void WindowManager::add(MainWindow *mainwin)
{
  this->windows.insert(mainwin);
  this->lastActive = mainwin;
}

void WindowManager::remove(MainWindow *mainwin)
{
  this->windows.remove(mainwin);
  if (this->lastActive == mainwin) {
    this->lastActive = this->windows.isEmpty() ? nullptr : *this->windows.begin();
  }
}

const QSet<MainWindow *>& WindowManager::getWindows() const
{
  return this->windows;
}

void WindowManager::setLastActive(MainWindow *mainwin)
{
  if (this->windows.contains(mainwin)) {
    this->lastActive = mainwin;
  }
}

MainWindow *WindowManager::getLastActive() const
{
  return this->lastActive;
}

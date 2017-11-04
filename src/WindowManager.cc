#include "WindowManager.h"
#include "MainWindow.h"

WindowManager::WindowManager()
{
}

WindowManager::~WindowManager()
{
}

void WindowManager::add(MainWindow *mainwin)
{
	this->windows.insert(mainwin);
}

void WindowManager::remove(MainWindow *mainwin)
{
	this->windows.remove(mainwin);
}

const QSet<MainWindow *> &WindowManager::getWindows() const
{
	return this->windows;
}

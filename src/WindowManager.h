#pragma once

#include <QObject>
#include <QSet>

class WindowManager : public QObject
{
	Q_OBJECT

public:
	WindowManager();
	~WindowManager();
	
	void add(class MainWindow *mainwin);
	void remove(class MainWindow *mainwin);
	const QSet<MainWindow*> &getWindows() const;
private:
	QSet<MainWindow*> windows;
};

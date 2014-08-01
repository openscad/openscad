#ifndef LAUNCHINGSCREEN_H
#define LAUNCHINGSCREEN_H

#include <QDialog>

namespace Ui {
class LaunchingScreen;
}

class LaunchingScreen : public QDialog
{
    Q_OBJECT

public:
    explicit LaunchingScreen(QWidget *parent = 0);
    ~LaunchingScreen();

    Ui::LaunchingScreen *ui;
};

#endif // LAUNCHINGSCREEN_H

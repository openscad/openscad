#pragma once

#include <QObject>
#include "gui/MainWindow.h"

class UXTest : public QObject
{
    Q_OBJECT;

public:
    void setWindow(MainWindow* window);

protected:
    MainWindow* window;
};

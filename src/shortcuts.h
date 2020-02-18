#pragma once
#include <QAction>
#include <QObject>
#include <QList>

class ShortCutConfigurator : public QObject
{
 Q_OBJECT 
public:
    ShortCutConfigurator();
    void apply(const QList<QAction *> &actions);
};

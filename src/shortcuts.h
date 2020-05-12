#pragma once
#include "qtgettext.h"
#include "ui_shortcuts.h"
#include <QAction>
#include <QObject>
#include <QList>
#include <QJsonValue>

class ShortCutConfigurator : public QWidget, public Ui::Shortcut
{
 Q_OBJECT 
public:
    ShortCutConfigurator(QWidget *parent = 0);
    virtual ~ShortCutConfigurator();
    void apply(const QList<QAction *> &actions);
};

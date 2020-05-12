#pragma once
#include "qtgettext.h"
#include "ui_shortcuts.h"
#include <QAction>
#include <QObject>
#include <QList>
#include <QJsonValue>

class ShortcutConfigurator : public QWidget, public Ui::Shortcut
{
 Q_OBJECT 
public:
    ShortcutConfigurator(QWidget *parent = 0);
    virtual ~ShortcutConfigurator();
    void apply(const QList<QAction *> &actions);
};

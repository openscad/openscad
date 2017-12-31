#pragma once

#include <QString>
#include <QAction>
#include <QDockWidget>

class Dock : public QDockWidget
{
	Q_OBJECT
	
public:
        Dock(QWidget *parent = nullptr);
        ~Dock();
        void setConfigKey(const QString configKey);
        void setAction(QAction *action);

public slots:
        void setVisible(bool visible) override;

private:
        QString configKey;
        QAction *action;
};

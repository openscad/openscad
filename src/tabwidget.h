#pragma once

#include <QTabBar>
#include <QStackedWidget>
#include <QList>

class TabWidget : public QTabBar
{
    Q_OBJECT

public:
    TabWidget(QWidget *parent = nullptr);
    ~TabWidget();
    QWidget *getContentWidget();

    int addTab(QWidget *w, const QString &label);
    int indexOf(QWidget *w);
    QWidget *widget(int index);
    void removeTab(int index);
    void setCurrentWidget(int index);
    void fireTabCountChanged();

private:
	QList<QWidget *> tabContent;
	QStackedWidget *stackWidget;

signals:
	void currentTabChanged(int);
    void tabCountChanged(int);

private slots:
	void handleCurrentChanged(int);
	void handleTabMoved(int, int);
};

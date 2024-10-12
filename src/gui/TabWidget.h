#pragma once

#include <QMouseEvent>
#include <QString>
#include <QWidget>
#include <QTabBar>
#include <QStackedWidget>
#include <QList>

class TabWidget : public QTabBar
{
  Q_OBJECT

public:
  TabWidget(QWidget *parent = nullptr);
  QWidget *getContentWidget();

  int addTab(QWidget *w, const QString& label);
  int indexOf(QWidget *w);
  QWidget *widget(int index);
  void removeTab(int index);
  void setCurrentWidget(int index);
  void fireTabCountChanged();
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  QList<QWidget *> tabContent;
  QStackedWidget *stackWidget;

signals:
  void currentTabChanged(int);
  void tabCountChanged(int);
  void middleMouseClicked(int);

private slots:
  void handleCurrentChanged(int);
  void handleTabMoved(int, int);
};

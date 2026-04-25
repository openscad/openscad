#pragma once

#include <QObject>
#include <QByteArray>
#include <QMainWindow>

class WorkspaceSaver : public QObject
{
  Q_OBJECT

public:
  static WorkspaceSaver *instance();

  void captureState(QMainWindow *window);
  void lock();
  void unlock();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
  void commitToSettings();

private:
  WorkspaceSaver(QObject *parent = nullptr);
  ~WorkspaceSaver() = default;

  void captureActiveMainWindow();

  QByteArray saved_geometry_;
  QByteArray saved_state_;
  bool locked_ = false;
};

#pragma once

#include "gui/qtgettext.h"
#include "ui_ProgressWidget.h"
#include <QWidget>
#include <QElapsedTimer>

class ProgressWidget : public QWidget, public Ui::ProgressWidget
{
  Q_OBJECT;
  Q_PROPERTY(bool wasCanceled READ wasCanceled);

public:
  ProgressWidget(QWidget *parent = nullptr);
  bool wasCanceled() const;
  int elapsedTime() const;

public slots:
  void setRange(int minimum, int maximum);
  void setValue(int progress);
  int value() const;
  void cancel();

signals:
  void requestShow();

private:
  bool wascanceled;
  QElapsedTimer starttime;
};

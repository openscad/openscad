#pragma once

#include <QElapsedTimer>
#include <QWidget>

#include "gui/qtgettext.h"
#include "ui_ProgressWidget.h"

class ProgressWidget : public QWidget, public Ui::ProgressWidget
{
  Q_OBJECT;
  Q_PROPERTY(bool wasCanceled READ wasCanceled);

public:
  ProgressWidget(QWidget *parent = nullptr);
  bool wasCanceled() const;
  int elapsedTime() const;
  void reset();

public slots:
  void setRange(int minimum, int maximum);
  void setValue(int progress);
  int value() const;
  void cancel();

private slots:
  void on_stopButton_clicked();

private:
  bool wascanceled;
  QElapsedTimer starttime;
};

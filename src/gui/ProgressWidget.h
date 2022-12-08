#pragma once

#include "qtgettext.h"
#include "ui_ProgressWidget.h"
#include <QElapsedTimer>

class ProgressWidget : public QWidget, public Ui::ProgressWidget
{
  Q_OBJECT;
  Q_PROPERTY(bool wasCanceled READ wasCanceled);

public:
  ProgressWidget(QWidget *parent = nullptr);
  [[nodiscard]] bool wasCanceled() const;
  [[nodiscard]] int elapsedTime() const;

public slots:
  void setRange(int minimum, int maximum);
  void setValue(int progress);
  [[nodiscard]] int value() const;
  void cancel();

signals:
  void requestShow();

private:
  bool wascanceled;
  QElapsedTimer starttime;
};

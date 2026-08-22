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
  // `showGuiProgress` marks an operation that has a GUI-side phase as well as a worker one --
  // i.e. a preview. Both bars are then shown from the start rather than the second appearing
  // partway through and shrinking the first.
  ProgressWidget(QWidget *parent = nullptr, bool showGuiProgress = false);
  bool wasCanceled() const;
  int elapsedTime() const;

public slots:
  void setRange(int minimum, int maximum);
  void setValue(int progress);
  int value() const;
  int guiValue() const;
  void startGuiProgress(int maximum);
  void setGuiValue(int progress);
  void cancel();

private slots:
  void on_stopButton_clicked();

signals:
  void requestShow();
  void canceled();

private:
  bool wascanceled;
  QElapsedTimer starttime;
};

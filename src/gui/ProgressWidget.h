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
  // Qt moc has problems with [[nodiscard]]
  bool wasCanceled() const; // NOLINT(modernize-use-nodiscard)
  int elapsedTime() const;  // NOLINT(modernize-use-nodiscard)

public slots:
  void setRange(int minimum, int maximum);
  void setValue(int progress);
  int value() const; // NOLINT(modernize-use-nodiscard)
  void cancel();

signals:
  void requestShow();

private:
  bool wascanceled;
  QElapsedTimer starttime;
};

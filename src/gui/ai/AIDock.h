
#pragma once

#include "gui/Dock.h"

class QWidget;
class QVBoxLayout;

class AIDock : public Dock
{
  Q_OBJECT

public:
  AIDock(QWidget *parent = nullptr);
  virtual ~AIDock();

protected:
  void showEvent(QShowEvent *event) override;

private:
  QWidget *centralWidget;
  QVBoxLayout *layout;
};

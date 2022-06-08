#pragma once

#include "qtgettext.h"
#include "ui_CameraControlWidget.h"
#include "printutils.h"
#include <QStandardItemModel>
#include "Editor.h"

class CameraControlWidget : public QWidget, public Ui::CameraControlWidget
{
  Q_OBJECT

public:
  CameraControlWidget(QWidget *parent = nullptr);
  CameraControlWidget(const CameraControlWidget& source) = delete;
  CameraControlWidget(CameraControlWidget&& source) = delete;
  CameraControlWidget& operator=(const CameraControlWidget& source) = delete;
  CameraControlWidget& operator=(CameraControlWidget&& source) = delete;
  void initGUI();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:


signals:
  void openFile(const QString, int);

private slots:

};

#pragma once

#include "qtgettext.h"
#include "ui_Animate.h"
#include <QIcon>

class MainWindow;

class Animate : public QWidget, public Ui::AnimateWidget
{
  Q_OBJECT

public:
  Animate(QWidget *parent = nullptr);
  Animate(const Animate& source) = delete;
  Animate(Animate&& source) = delete;
  Animate& operator=(const Animate& source) = delete;
  Animate& operator=(Animate&& source) = delete;
  void initGUI();
  bool dumpPictures();
  int nextFrame();

  QTimer *animate_timer;

  void setMainWindow(MainWindow *mainWindow);
  MainWindow *mainWindow;

public slots:
  void animateUpdate();
  void updatedAnimFps();

protected:
//  void resizeEvent(QResizeEvent *event) override;


private:

  void updatePauseButtonIcon();

  double anim_tval;
  bool anim_dumping;
  int anim_dump_start_step;
  int anim_step;
  int anim_numsteps;

  bool isLightTheme();
  
signals:

private slots:
  void updatedAnimTval();

  void updatedAnimSteps();
  void updatedAnimDump(bool checked);
  void updateTVal();
  void on_pauseButton_pressed();

};

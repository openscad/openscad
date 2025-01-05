#pragma once

#include <QAction>
#include <QList>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QWidget>
#include <string>

#include "gui/qtgettext.h"
#include "ui_Animate.h"
#include <QIcon>
#include "gui/input/InputDriverEvent.h"

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
  ~Animate() override = default;

  void initGUI();
  bool dumpPictures();
  int nextFrame();

  QTimer *animate_timer;

  void setMainWindow(MainWindow *mainWindow);

  const QList<QAction *>& actions();
  double getAnim_tval();

public slots:
  void animateUpdate();
  void cameraChanged();
  void editorContentChanged();
  void onActionEvent(InputEventAction *event);
  void pauseAnimation();

  void on_pushButton_MoveToBeginning_clicked();
  void on_pushButton_StepBack_clicked();
  void on_pushButton_StepForward_clicked();
  void on_pushButton_MoveToEnd_clicked();

protected:
  void resizeEvent(QResizeEvent *event) override;


private:
  MainWindow *mainWindow;

  void updatePauseButtonIcon();
  void connectAction(QAction *, QPushButton *);

  double anim_tval;
  bool anim_dumping;
  int anim_dump_start_step;
  int anim_step;
  int anim_numsteps;

  bool fps_ok;
  bool t_ok;
  bool steps_ok;

  int initMinWidth;

  QIcon iconRun;
  QIcon iconPause;
  QIcon iconDisabled;
  QList<QAction *> action_list;
  QColor errorBlendColor{"red"};

signals:

private slots:
  void updatedAnimTval();
  void updatedAnimFpsAndAnimSteps();
  void updatedAnimDump(bool checked);
  void incrementTVal();
  void updateTVal();
  void on_pauseButton_pressed();
};

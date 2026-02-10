#pragma once

#include <QColor>
#include <QString>
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

  QTimer *animateTimer;

  void setMainWindow(MainWindow *mainWindow);

  const QList<QAction *>& actions();
  double getAnimTval();

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

  double animTVal;
  bool animDumping;
  int animDumpStartStep;
  int animStep;
  int animNumSteps;

  bool fpsOK;
  bool tOK;
  bool steps_ok;

  int initMinWidth;

  QIcon iconRun;
  QIcon iconPause;
  QIcon iconDisabled;
  QList<QAction *> actionList;
  QColor errorBlendColor{"red"};

signals:

private slots:
  void on_e_tval_textChanged(const QString&);
  void on_e_fps_textChanged(const QString&);
  void on_e_fsteps_textChanged(const QString&);
  void on_e_dump_toggled(bool checked);
  void updatedAnimFpsAndAnimSteps();
  void incrementTVal();
  void updateTVal();
  void on_pauseButton_pressed();
};

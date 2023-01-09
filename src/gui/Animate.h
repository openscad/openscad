#pragma once

#include "qtgettext.h"
#include "ui_Animate.h"
#include <QIcon>
#include "input/InputDriverEvent.h"

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
  QSize minimumSizeHint() const override;

public slots:
  void animateUpdate();
  void cameraChanged();
  void editorContentChanged();
  void onActionEvent(InputEventAction *event);
  void pauseAnimation();

  void on_pushButton_MoveToBeginning_clicked();
  void on_pushButton_StepBack_clicked();
  void on_pushButton_Resume_clicked();
  void on_pushButton_Pause_clicked();
  void on_pushButton_StepForward_clicked();
  void on_pushButton_MoveToEnd_clicked();

protected:
  void resizeEvent(QResizeEvent *event) override;


private:
  MainWindow *mainWindow;

  void updatePauseButtonIcon();
  void initVCR();

  double anim_tval;
  bool anim_dumping;
  int anim_dump_start_step;
  int anim_step;
  int anim_numsteps;

  bool isLightTheme();

  bool fps_ok;
  bool t_ok;
  bool steps_ok;

  QList<QAction *> action_list;
  void createActionAndPrepareButton(const QIcon& icon, const QString& description, const std::string& actionName, QPushButton *button);

signals:

private slots:
  void updatedAnimTval();
  void updatedAnimFpsAndAnimSteps();
  void updatedAnimDump(bool checked);
  void incrementTVal();
  void updateTVal();
  void on_pauseButton_pressed();
};

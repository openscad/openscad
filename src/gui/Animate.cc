#include "Animate.h"
#include "printutils.h"
#include "MainWindow.h"
#include <boost/filesystem.hpp>
#include <QFormLayout>
#include "OffscreenView.h"

Animate::Animate(QWidget *parent) : QWidget(parent)
{
  setupUi(this);
  initGUI();
}

void Animate::initGUI()
{
  this->anim_step = 0;
  this->anim_numsteps = 0;
  this->anim_tval = 0.0;
  this->anim_dumping = false;
  this->anim_dump_start_step = 0;

  animate_timer = new QTimer(this);
  connect(animate_timer, SIGNAL(timeout()), this, SLOT(incrementTVal()));

  connect(this->e_tval, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimTval()));
  connect(this->e_fps, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimFpsAndAnimSteps()));
  connect(this->e_fsteps, SIGNAL(textChanged(QString)), this, SLOT(updatedAnimFpsAndAnimSteps()));
  connect(this->e_dump, SIGNAL(toggled(bool)), this, SLOT(updatedAnimDump(bool)));
}

void Animate::setMainWindow(MainWindow *mainWindow)
{
  this->mainWindow = mainWindow;

  //prepare actions for inputdriver
  QIcon playIcon = isLightTheme() ? QIcon(":/icons/svg-default/animate.svg") : QIcon(":/icons/svg-default/animate-white.svg");
  QIcon pauseIcon = isLightTheme() ? QIcon(":/icons/svg-default/animate_pause.svg") : QIcon(":/icons/svg-default/animate_pause-white.svg");

  createActionAndPrepareButton(
    playIcon, _("toogle pause/unpause"),
    "pauseUnpause", pauseButton
    );

  initVCR();
  updatePauseButtonIcon();
}

void Animate::createActionAndPrepareButton(const QIcon &icon, QString description, std::string actionName, QPushButton* button){
  QAction *action = new QAction(icon, description, this);
  action->setObjectName(QString::fromStdString(actionName));

  connect(action, SIGNAL(triggered()), button, SLOT(click()));
  this->action_list.append(action);

  button->setIcon(icon);
  button->setToolTip(description);
  button->setText("");
}

void Animate::initVCR(){
  QString suffix("");
  if(!isLightTheme()){
    suffix = QString("-white");
  }
  static QIcon startIcon    = QIcon(":/icons/svg-default/vcr-control-start" + suffix + ".svg");
  static QIcon stepBackIcon = QIcon(":/icons/svg-default/vcr-control-step-back" + suffix + ".svg");
  static QIcon playIcon     = QIcon(":/icons/svg-default/vcr-control-play" + suffix + ".svg");
  static QIcon pauseIcon    = QIcon(":/icons/svg-default/vcr-control-pause" + suffix + ".svg");
  static QIcon stepFwrdIcon = QIcon(":/icons/svg-default/vcr-control-step-forward" + suffix + ".svg");
  static QIcon endIcon      = QIcon(":/icons/svg-default/vcr-control-end" + suffix + ".svg");

  createActionAndPrepareButton(
    startIcon, _("Move to beginning (first frame)"),
    "start", pushButton_MoveToBeginning);

  createActionAndPrepareButton(
    stepBackIcon, _("step one frame back"),
    "stepBack", pushButton_StepBack);

  createActionAndPrepareButton(
    playIcon, _("play animation"),
    "play", pushButton_Resume);

  createActionAndPrepareButton(
    pauseIcon, _("pause animation"),
    "pause", pushButton_Pause);

  createActionAndPrepareButton(
    stepFwrdIcon, _("step one frame forward"),
    "stepFwrd", pushButton_StepForward);

  createActionAndPrepareButton(
    endIcon,_("Move to end (last frame)"),
    "end", pushButton_MoveToEnd);
}

bool Animate::isLightTheme()
{
  bool ret = true;
  if(mainWindow){
    ret = mainWindow->isLightTheme();
  } else {
    std::cout << "Animate: You need to set the mainWindow before calling isLightTheme" << std::endl;
  }
  return ret;
}

void Animate::updatedAnimTval()
{
  bool ownsMutex = anim_step_Mutex.try_lock();

  double t = this->e_tval->text().toDouble(&this->t_ok);
  // Clamp t to 0-1
  if (this->t_ok) {
    t = t < 0 ? 0.0 : ((t > 1.0) ? 1.0 : t);
  } else {
    t = 0.0;
  }
  if(ownsMutex){
    anim_step = (int)(anim_numsteps * t);
  }
  horizontalSlider->setValue(anim_step);
  horizontalSlider->setMaximum(anim_numsteps);

  this->anim_tval = t;
  emit mainWindow->actionRenderPreview();

  updatePauseButtonIcon();
  
  if(ownsMutex) anim_step_Mutex.unlock();
}

void Animate::updatedAnimFpsAndAnimSteps()
{
  animate_timer->stop();

  int numsteps = this->e_fsteps->text().toInt(&this->steps_ok);
  if (this->steps_ok) {
    this->anim_numsteps = numsteps;
  } else {
    this->anim_numsteps = 0;
  }
  this->anim_dumping = false;

  double fps = this->e_fps->text().toDouble(&this->fps_ok);
  animate_timer->stop();
  if (this->fps_ok && fps > 0 && this->anim_numsteps > 0) {
    this->anim_step = int(this->anim_tval * this->anim_numsteps) % this->anim_numsteps;
    animate_timer->setSingleShot(false);
    animate_timer->setInterval(int(1000 / fps));
    animate_timer->start();
  }

  QString redBackground = QString (isLightTheme() ? "background-color:#ffaaaa;" : "background-color:#502020;");

  if( this->steps_ok || this->e_fsteps->text()=="" ){
    this->e_fsteps->setStyleSheet(""); 
  } else {
    this->e_fsteps->setStyleSheet(redBackground); 
  }

  if( this->fps_ok || this->e_fps->text()=="" ){
    this->e_fps->setStyleSheet(""); 
  } else {
    this->e_fps->setStyleSheet(redBackground); 
  }

  updatePauseButtonIcon();
}


void Animate::updatedAnimDump(bool checked)
{
  if (!checked) this->anim_dumping = false;

  updatePauseButtonIcon();
}

// Only called from animate_timer
void Animate::incrementTVal()
{
  bool ownsMutex = anim_step_Mutex.try_lock();

  if (this->anim_numsteps == 0) return;

  if (mainWindow->windowActionHideCustomizer->isVisible()) {
    if (mainWindow->activeEditor->parameterWidget->childHasFocus()) return;
  }

  if (this->anim_numsteps > 1) {
    this->anim_step = (this->anim_step + 1) % this->anim_numsteps;
    this->anim_tval = 1.0 * this->anim_step / this->anim_numsteps;
  } else if (this->anim_numsteps > 0) {
    this->anim_step = 0;
    this->anim_tval = 0.0;
  }

  const QString txt = QString::number(this->anim_tval, 'f', 5);
  this->e_tval->setText(txt);

  updatePauseButtonIcon();
  
  if(ownsMutex) anim_step_Mutex.unlock();
}

void Animate::updateTVal()
{
  if (this->anim_numsteps == 0) return;

  if(this->anim_step < 0){
    this->anim_step = this->anim_numsteps - this->anim_step - 2;
  }

  if (this->anim_numsteps > 1) {
    this->anim_step =      (this->anim_step) % this->anim_numsteps;
    this->anim_tval = 1.0 * this->anim_step  / this->anim_numsteps;
  } else if (this->anim_numsteps > 0) {
    this->anim_step = 0;
    this->anim_tval = 0.0;
  }

  const QString txt = QString::number(this->anim_tval, 'f', 5);
  this->e_tval->setText(txt);

  updatePauseButtonIcon();
}

void Animate::pauseAnimation(){
  animate_timer->stop();
  updatePauseButtonIcon();
}

void Animate::on_pauseButton_pressed()
{
  if (animate_timer->isActive()) {
    animate_timer->stop();
    updatePauseButtonIcon();
  } else {
    this->updatedAnimFpsAndAnimSteps();
  }
}

void Animate::updatePauseButtonIcon()
{
  static QIcon runDark(":/icons/svg-default/animate.svg");
  static QIcon runLight(":/icons/svg-default/animate-white.svg");

  static QIcon pauseDark(":/icons/svg-default/animate_pause.svg");
  static QIcon pauseLight(":/icons/svg-default/animate_pause-white.svg");

  static QIcon disabledDark(":/icons/svg-default/animate_disabled.svg");
  static QIcon disabledLight(":/icons/svg-default/animate_disabled-white.svg");

  if (animate_timer->isActive()) {
    pauseButton->setIcon( this->isLightTheme() ? pauseDark : pauseLight );
    pauseButton->setToolTip( _("press to pause animation") );
  } else {
    if( this->fps_ok && this->steps_ok ){
      pauseButton->setIcon( this->isLightTheme() ? runDark : runLight );
      pauseButton->setToolTip( _("press to start animation") );
    } else {
      pauseButton->setIcon( this->isLightTheme() ? disabledDark : disabledLight );
      pauseButton->setToolTip( _("incorrect values") );
    }
  }
}

void Animate::cameraChanged(){
  this->animateUpdate(); //for now so that we do not change the behavior
}

void Animate::editorContentChanged(){
  this->animateUpdate(); //for now so that we do not change the behavior
}

void  Animate::animateUpdate()
{
  if (mainWindow->animateDockContents->isVisible()) {
    double fps = this->e_fps->text().toDouble(&this->fps_ok);
    if (this->fps_ok && fps <= 0 && !animate_timer->isActive()) {
      animate_timer->stop();
      animate_timer->setSingleShot(true);
      animate_timer->setInterval(50);
      animate_timer->start();
    }
  }
}

void Animate::dumpPicture(){
  if( this->e_dump->isChecked() && this->animate_timer->isActive() ){
      int steps = this->nextFrame();
      QString filename = QString("frame%1.png").arg(steps, 5, 10, QChar('0'));
      if(this->checkBox_offscreen->isChecked()){
        int w = spinBox_offScreenWidth->value();
        int h = spinBox_offScreenHeight->value();
        OffscreenView offscreenView(w, h);

        QString filename = QString("frame%1.png").arg(steps, 5, 10, QChar('0'));

        auto qglview = mainWindow->qglview;
        offscreenView.setCamera(qglview->cam);
        offscreenView.setRenderer(qglview->getRenderer());
        offscreenView.setColorScheme(*(qglview->colorscheme));
        offscreenView.setShowFaces(qglview->showFaces());
        offscreenView.setShowCrosshairs(qglview->showCrosshairs());
        offscreenView.setShowAxes(qglview->showAxes());
        offscreenView.setShowScaleProportional(qglview->showScaleProportional());
        offscreenView.setShowEdges(qglview->showEdges());

        offscreenView.paintGL();

        offscreenView.save(filename.toStdString().c_str());
      } else {
        QImage img = mainWindow->qglview->grabFrame();
        img.save(filename, "PNG");
    }
  }  
}

int Animate::nextFrame(){
  if (anim_dumping && anim_dump_start_step == anim_step) {
      anim_dumping = false;
      e_dump->setChecked(false);
    } else {
      if (!anim_dumping) {
        anim_dumping = true;
        anim_dump_start_step = anim_step;
      }
  }
  return anim_step;
}

// Invalid minimumSizeHint means we accept any size.
// This is not ideal, but  QT does not seam to have an
// elegant way to handle widgets that can adapt
// to horizontal and vertical layout
QSize Animate::minimumSizeHint() const{
    return QSize(-1,-1);
}

void Animate::resizeEvent(QResizeEvent *event)
{
  const QSize sizeEvent = size();
  
  // QTDesigner does not make it obvious, but
  // QBoxLayout can be switch from vertical to horizontal.
  int iconSize = 16;
  if(auto mainLayout = dynamic_cast<QBoxLayout *> (this->layout())){
    if(sizeEvent.height() > 180){
      mainLayout->setDirection(QBoxLayout::TopToBottom);
      if(sizeEvent.height() > 250 && sizeEvent.width() > 200){
        mainLayout->setMargin(10);
        mainLayout->setSpacing(10);
        iconSize = 32;
      } else {
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);
      }
      this->horizontalSlider->show();
      this->vcr_controls->show();
      this->offscreenSettingGroupBox->show();
    } else {
      mainLayout->setDirection(QBoxLayout::LeftToRight);

      mainLayout->setMargin(0);
      mainLayout->setSpacing(0);
      if(sizeEvent.width() > 720){
        this->vcr_controls->show();
      } else {
        this->vcr_controls->hide();
      }
      this->horizontalSlider->hide();
      this->offscreenSettingGroupBox->hide();
    }
  } else {
    static bool warnOnce = true;
    if(warnOnce) {
      std::cout << "you should not see this message - "
                << " if you work on the animate UI, you can consider removing this code"
                << std::endl;
      warnOnce = false;
    }
  }

  auto qPushButtons = this->findChildren<QPushButton*>();
  for(auto qPushButton : qPushButtons){
      qPushButton->setIconSize(QSize(iconSize, iconSize));
  }
  
  QFormLayout::RowWrapPolicy policy = QFormLayout::RowWrapPolicy::DontWrapRows;
  if(sizeEvent.width() < 150){
    policy = QFormLayout::RowWrapPolicy::WrapAllRows;
  }
  auto qFormLayouts = this->findChildren<QFormLayout*>();
  for(auto qFormLayout : qFormLayouts){
      qFormLayout->setRowWrapPolicy(policy);
  }

  QWidget::resizeEvent(event);
}

const QList<QAction *>& Animate::actions(){
  return action_list;
}

void Animate::onActionEvent(InputEventAction *event)
{
  const std::string actionString = event->action;
  const std::string target = actionString.substr(0, actionString.find("::"));
  const std::string actionName = actionString.substr(actionString.find("::")+2, std::string::npos);
  for(auto action : action_list){
    if(actionName == action->objectName().toStdString()){
        action->trigger();
    }
  }
}

double Animate::getAnim_tval(){
  return anim_tval;
}

void Animate::on_pushButton_MoveToBeginning_clicked(){
  pauseAnimation();
  this->anim_step = 0;
  this->updateTVal();
}

void Animate::on_pushButton_StepBack_clicked(){
  pauseAnimation();
  this->anim_step -= 1;
  this->updateTVal();
}

void Animate::on_pushButton_Resume_clicked(){
  updatedAnimFpsAndAnimSteps();
}

void Animate::on_pushButton_Pause_clicked(){
  pauseAnimation();
}

void Animate::on_pushButton_StepForward_clicked(){
  pauseAnimation();
  this->anim_step += 1;
  this->updateTVal();
}

void Animate::on_pushButton_MoveToEnd_clicked(){
  pauseAnimation();
  this->anim_step = this->anim_numsteps - 1;
  this->updateTVal();
}

void Animate::on_horizontalSlider_valueChanged(int val){
  if(!anim_step_Mutex.try_lock()) return;
  this->anim_step  = val;
  updateTVal();
  anim_step_Mutex.unlock();
}
void Animate::on_comboBoxResolution_currentIndexChanged(int index){
  if(index == 0) return;
  bool ownsLock = anim_resolution_Mutex.try_lock();
  std::string current  = comboBoxResolution->currentText().toStdString();

  size_t start = current.find("(")+1;
  size_t stop  = current.find("x")-1;
  std::string width= current.substr(start,stop-start);

  start = current.find("x")+1;
  stop  = current.find("px)");
  std::string height= current.substr(start,stop-start);

  try
  {
    int w = std::stoi(width);
    int h = std::stoi(height);
    spinBox_offScreenWidth->setValue(w);
    spinBox_offScreenHeight->setValue(h);
  }catch(std::exception const& ex){
  }
  if(ownsLock) anim_resolution_Mutex.unlock();
}

void Animate::on_spinBox_offScreenWidth_valueChanged(int){
  bool ownsLock = anim_resolution_Mutex.try_lock();
  checkBox_offscreen->setCheckState(Qt::Checked);

  if(ownsLock) comboBoxResolution->setCurrentIndex(0);
  if(ownsLock) anim_resolution_Mutex.unlock();
}

void Animate::on_spinBox_offScreenHeight_valueChanged(int){
  bool ownsLock = anim_resolution_Mutex.try_lock();
  checkBox_offscreen->setCheckState(Qt::Checked);

  if(ownsLock) comboBoxResolution->setCurrentIndex(0);
  if(ownsLock) anim_resolution_Mutex.unlock();
}

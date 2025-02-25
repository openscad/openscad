#pragma once

#include <QRubberBand>

class Dock;
class MainWindow;

///
/// A rubberband is a qt concept to draw an overlay over widgets.
/// In our case it is used to emphasize which doc is currently selected
///
class RubberBandManager : QObject
{
  Q_OBJECT

public:
  RubberBandManager(MainWindow *w);

  void hide();
  void emphasize(Dock *w);


private:
  bool eventFilter(QObject *obj, QEvent *event) final override;
  QRubberBand rubberBand;
};

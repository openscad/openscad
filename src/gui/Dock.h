#pragma once

#include <QAction>
#include <QDockWidget>
#include <QString>

class Dock : public QDockWidget
{
  Q_OBJECT

public:
  Dock(QWidget *parent = nullptr);
  virtual ~Dock();

  void setName(const QString& name_);
  [[nodiscard]] QString getName() const;

  void setNameSuffix(const QString& namesuffix_);
  void setTitleBarVisibility(bool isVisible);
  void updateTitle();

public slots:
  void onTopLevelStatusChanged(bool);

private:
  QString name;
  QString namesuffix;
  QWidget *dockTitleWidget;
};

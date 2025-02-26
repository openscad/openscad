#pragma once

#include <QString>
#include <QAction>
#include <QDockWidget>

class Dock : public QDockWidget
{
  Q_OBJECT

public:
  Dock(QWidget *parent = nullptr);
  virtual ~Dock();

  void setConfigKey(const QString& configKey);
  void disableSettingsUpdate();

  void setName(const QString& name_);
  [[nodiscard]] QString getName() const;

  void setNameSuffix(const QString& namesuffix_);
  void setTitleBarVisibility(bool isVisible);
  void updateTitle();

public slots:
  void setVisible(bool visible) override;
  void onTopLevelStatusChanged(bool);

private:
  QString name;
  QString namesuffix;
  QString configKey;
  bool updateSettings{true};
  QWidget *dockTitleWidget;
};

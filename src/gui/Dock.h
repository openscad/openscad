#pragma once

#include <QString>
#include <QAction>
#include <QDockWidget>

class Dock : public QDockWidget
{
  Q_OBJECT

public:
  Dock(QWidget *parent = nullptr);
  void setConfigKey(const QString& configKey);
  void setAction(QAction *action);
  void disableSettingsUpdate();

  void setName(const QString& name_);
  [[nodiscard]] QString getName() const;

  void setNameSuffix(const QString& namesuffix_);
  void updateTitle();

public slots:
  void setVisible(bool visible) override;
  void onTopLevelStatusChanged(bool);

private:
  QString name;
  QString namesuffix;
  QString configKey;
  QAction *action{nullptr};
  bool updateSettings{true};
};

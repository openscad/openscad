#pragma once
#include "qtgettext.h"
#include "ui_ShortcutConfigurator.h"
#include "PlatformUtils.h"
#include "printutils.h"
#include <QAction>
#include <QObject>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QHash>
#include <QRegExp>
#include <QMessageBox>

class ShortcutConfigurator : public QWidget, public Ui::ShortcutConfigurator
{
 Q_OBJECT 
public:
    ShortcutConfigurator(QWidget *parent = 0);
    ShortcutConfigurator(const ShortcutConfigurator& source) = delete;
    ShortcutConfigurator(ShortcutConfigurator&& source) = delete;
    ShortcutConfigurator& operator=(const ShortcutConfigurator& source) = delete;
    ShortcutConfigurator& operator=(ShortcutConfigurator&& source) = delete;
    virtual ~ShortcutConfigurator();
    void collectDefaults(const QList<QAction *> &allActions);
    void initGUI(const QList<QAction *> &allActions);
    void applyConfigFile(const QList<QAction *> &actions);
    void updateShortcut(QAction* changedAction,QString updatedShortcut,const QModelIndex & index);
    void resetClass();

private:
    void createModel(QObject* parent,const QList<QAction *> &actions);
    void readConfigFile(QJsonObject *object);
    bool writeToConfigFile(QJsonObject *object);
    void raiseError(const QString errorMsg);
    QString getData(int row,int col);
    void putData(QModelIndex indexA,QString data);
    std::string configFileLoc;
    QMultiHash<QString, QAction *> shortcutsMap;
    QHash<QString, QString> shortcutOccupied;
    QList<QString> actionsName;
    QMap<QAction*,QList<QKeySequence>> defaultShortcuts;
    QList<QAction *> actionsList;
    QKeySequence pressedKeySequence;
    QMessageBox *shortcutCatcher;
    QStandardItemModel* model;

protected:
   bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void onTableCellClicked(const QModelIndex & index);
    void on_searchBox_textChanged(const QString &arg1);
    void on_reset_clicked();
};

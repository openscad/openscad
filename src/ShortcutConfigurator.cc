#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QVariant>
#include "ShortcutConfigurator.h"
#include "PlatformUtils.h"
#include "printutils.h"
#include "qtgettext.h"
#include <QAbstractItemModel>
#include <QHeaderView>

ShortcutConfigurator::ShortcutConfigurator(QWidget *parent): QWidget(parent)
{
    setupUi(this);
}

ShortcutConfigurator::~ShortcutConfigurator()
{
    
}


QStandardItemModel* ShortcutConfigurator::createModel(QObject* parent,const QList<QAction *> &actions)
{
    const int numRows = actions.size();
    const int numColumns = 3;
    int row = 0;
    QStandardItemModel* model = new QStandardItemModel(numRows, numColumns);
    QList<QString> labels = QList<QString>() << QString("Action") << QString("Shortcut")<<QString("Alternative"); 
    model->setHorizontalHeaderLabels(labels);
    for (auto &action : actions) 
    {
        QString actionName = action->objectName();
        if(!actionName.isEmpty())
        {
        QStandardItem* actionNameItem = new QStandardItem(actionName);
        model->setItem(row, 0, actionNameItem);
        actionNameItem->setFlags(actionNameItem->flags() &  ~Qt::ItemIsEditable);

        const QString shortCut(action->shortcut().toString(QKeySequence::NativeText));
        QStandardItem* shortcutItem = new QStandardItem(shortCut);
        model->setItem(row, 1, shortcutItem);
        this->shortcutsMap.insert(actionName,action);
        row++;
        }
    }
    return model;
}

void ShortcutConfigurator::initGUI(const QList<QAction *> &allActions)
{
    QTableView *shortcutTableView = this->findChild<QTableView *>();
    shortcutTableView->verticalHeader()->hide();
    shortcutTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    initTable(shortcutTableView,allActions);
    connect(shortcutTableView->model(),SIGNAL(dataChanged(QModelIndex,QModelIndex)),SLOT(UpdateData(QModelIndex,QModelIndex)));

}

void ShortcutConfigurator::initTable(QTableView *shortcutsTable,const QList<QAction *> &allActions)
{
    shortcutsTable->setModel(createModel(shortcutsTable,allActions));
}

void ShortcutConfigurator::apply(const QList<QAction *> &actions)
{
	std::string absolutePath = PlatformUtils::userConfigPath()+"/shortcuts.json";

	QString finalPath = QString::fromLocal8Bit(absolutePath.c_str());

	QFile jsonFile(finalPath);
	
    //check if a User-Defined Shortcuts file exists or not
	if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	return;
	}

	QByteArray jsonData = jsonFile.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(jsonData);
	QJsonObject object = doc.object();


    for (auto &action : actions) 
    {
            
        QString actionName = action->objectName();
        if(!actionName.isEmpty())
        {
            QList<QKeySequence> shortcutsListFinal;

            //check if the actions new shortcut exists in the file or not
            //This would allow users to remove any default shortcut, by just leaving the key's value for an action name as empty.
            QJsonObject::const_iterator i = object.find(actionName);
            if(i != object.end() && i.key() == actionName)
            {
                QJsonValue val = object.value(actionName);
                //check if it is an array or not
                if(!val.isArray())
                {
                    QString singleShortcut = val.toString().trimmed();
                    action->setShortcut(QKeySequence(singleShortcut));
                }
                else
                {

                //create a key sequence
                QJsonArray array = val.toArray();
                foreach (const QJsonValue & v, array)
                {
                    QString shortcut = v.toString();
                    if(shortcut.isEmpty()) continue;
                    const QString shortCut(action->shortcut().toString(QKeySequence::NativeText));
                    if (!QString::compare(shortcut, "DEFAULT", Qt::CaseInsensitive))
                    {
                        if(shortCut.isEmpty()) continue;
                        shortcutsListFinal.append(shortCut);
                    }
                    else {
                    shortcutsListFinal.append(shortcut);
                    }
                }

                action->setShortcuts(shortcutsListFinal);
                }   
            }
        }
    }

}

QString ShortcutConfigurator::getData(int row,int col)
{
    QTableView *shortcutTableView = this->findChild<QTableView *>();
    return shortcutTableView->model()->index(row,col).data().toString();
}

void ShortcutConfigurator::UpdateData(const QModelIndex & indexA, const QModelIndex & indexB)
{
    QString updatedAction = getData(indexA.row(),0);

    auto itr = this->shortcutsMap.find(updatedAction);
    QAction* changedAction = itr.value();

    QString updatedShortcut = getData(indexA.row(),indexA.column());

    QList<QKeySequence> shortcutsListFinal;

    if(indexA.column()==1)
    {
 
        // primary shortcut

        // get the primary (updatedShortcut)
        shortcutsListFinal.append(updatedShortcut);

        // check if secondary already exists , and add it to sequence
        QString secondaryShortcut = getData(indexA.row(),2);
        if(!secondaryShortcut.isEmpty())
        {
            shortcutsListFinal.append(secondaryShortcut);
            changedAction->setShortcuts(shortcutsListFinal);
        }
        else
        {
            changedAction->setShortcut(QKeySequence(updatedShortcut));
        }


    }
    else if(indexA.column()==2)
    {
        // alternative shortcut

        //check if primary is defined
        QString primaryShortcut = getData(indexA.row(),1);
        if(!primaryShortcut.isEmpty())
        {
        shortcutsListFinal.append(primaryShortcut);
        shortcutsListFinal.append(updatedShortcut);
        changedAction->setShortcuts(shortcutsListFinal);
        }

    }

 
    // write into the file



}
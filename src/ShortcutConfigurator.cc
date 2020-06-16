#include "ShortcutConfigurator.h"
#include <QFile>
#include <QVariant>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QMessageBox>

ShortcutConfigurator::ShortcutConfigurator(QWidget *parent): QWidget(parent)
{
    setupUi(this);
}

ShortcutConfigurator::~ShortcutConfigurator()
{
    
}

void ShortcutConfigurator::collectDefaults(const QList<QAction *> &allActions)
{
    for(auto &action:allActions) 
    {
        defaultShortcuts.insert(action,action->shortcuts());
        QString actionName = action->objectName();
        if(!shortcutsMap.contains(actionName)) 
        {
            actionsName.push_back(actionName);
            actionsList.push_back(action);
            shortcutsMap.insert(actionName,action);
        }
    }
}

QStandardItemModel* ShortcutConfigurator::createModel(QObject* parent,const QList<QAction *> &actions)
{
    const int numRows = 0;
    const int numColumns = 3;
    int row = 0;
    QStandardItemModel* model = new QStandardItemModel(numRows, numColumns);
    QList<QString> labels = QList<QString>() << QString("Action") << QString("Shortcut")<<QString("Alternative-1"); 
    model->setHorizontalHeaderLabels(labels);
    for (auto &action : actions) 
    {
        QString actionName = action->objectName();
        if(!actionName.isEmpty())
        {
            QStandardItem* actionNameItem = new QStandardItem(actionName);
            model->setRowCount(row+1);
            model->setItem(row, 0, actionNameItem);
            actionNameItem->setFlags(actionNameItem->flags() &  ~Qt::ItemIsEditable);


            const QList<QKeySequence> shortcutsList = action->shortcuts();
            int index = 1;
            for(auto &shortcutSeq : shortcutsList)
            {
                const QString shortcut = shortcutSeq.toString(QKeySequence::NativeText);
                QStandardItem* shortcutItem = new QStandardItem(shortcut);
                if(index>2)
                {
                    model->setColumnCount(index+1);
                    QString label = QStringLiteral("Alternative-%1").arg(index-1);
                    labels.push_back(label);
                    model->setHorizontalHeaderLabels(labels);
                }
                model->setItem(row, index, shortcutItem);
                index++;
            }
            row++;
        }
    }
    return model;
}

void ShortcutConfigurator::initGUI(const QList<QAction *> &allActions)
{
    shortcutsTable->verticalHeader()->hide();
    shortcutsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    initTable(shortcutsTable,allActions);
    connect(shortcutsTable->model(),SIGNAL(dataChanged(QModelIndex,QModelIndex)),SLOT(updateShortcut(QModelIndex,QModelIndex)));
}

void ShortcutConfigurator::initTable(QTableView *shortcutsTable,const QList<QAction *> &allActions)
{
    shortcutsTable->setModel(createModel(shortcutsTable,allActions));
}

void ShortcutConfigurator::applyConfigFile(const QList<QAction *> &actions)
{
    QJsonObject object;
    readConfigFile(&object);    

    for (auto &action : actions) 
    {
            
        QString actionName = action->objectName();
        if(!actionName.isEmpty())
        {
            QList<QKeySequence> shortcutsListFinal;

            // check if the actions new shortcut exists in the file or not
            // This would allow users to remove any default shortcut, by just leaving the key's value for an action name as empty.
            QJsonObject::const_iterator i = object.find(actionName);
            if(i != object.end() && i.key() == actionName)
            {
                QJsonValue val = object.value(actionName);
                // check if it is an array or not
                if(!val.isArray())
                {
                    QString singleShortcut = val.toString().trimmed();
                    if(shortcutOccupied[singleShortcut]==true)
                    {
                        raiseError(QString("Shortcut Config-File: Shortcut Already Occupied"));
                        return;
                    }
                    else if(singleShortcut!=QString::fromUtf8("")) 
                    {
                        shortcutOccupied.insert(singleShortcut,true);
                    }
                    action->setShortcut(QKeySequence(singleShortcut));
                }
                else
                {
                    // create a key sequence
                    QJsonArray array = val.toArray();
                    foreach (const QJsonValue & v, array)
                    {
                        QString shortcut = v.toString();
                        if(shortcut==QString::fromUtf8("")) continue;
                        const QString shortCut(action->shortcut().toString(QKeySequence::NativeText));
                        if (!QString::compare(shortcut, "DEFAULT", Qt::CaseInsensitive))
                        {
                            if(shortCut==QString::fromUtf8("")) continue;
                            shortcutOccupied.insert(shortCut,true);
                            shortcutsListFinal.append(shortCut);
                        }
                        else 
                        {
                            if(shortcutOccupied[shortcut]==true)
                            {
                                raiseError(QString("Shortcut Config-File: Shortcut Already Occupied"));
                                return;
                            }
                            else if(shortcut!=QString::fromUtf8("")) 
                            {
                                shortcutOccupied.insert(shortcut,true);
                            }
                            shortcutsListFinal.append(shortcut);
                        }
                    }

                    action->setShortcuts(shortcutsListFinal);
                }   
            }
        }
    }

}

void ShortcutConfigurator::readConfigFile(QJsonObject* object)
{
    std::string absolutePath = PlatformUtils::userConfigPath()+"/shortcuts.json";
    QString finalPath = QString::fromLocal8Bit(absolutePath.c_str());

    QFile jsonFile(finalPath);

    // check if a User-Defined Shortcuts file exists or not
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return ;
    }

    QByteArray jsonData = jsonFile.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    *object = doc.object();
}

bool ShortcutConfigurator::writeToConfigFile(QJsonObject* object)
{

    std::string absolutePath = PlatformUtils::userConfigPath()+"/shortcuts.json";
    QString finalPath = QString::fromLocal8Bit(absolutePath.c_str());

    QFile jsonFile(finalPath);

    if (!jsonFile.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QJsonDocument doc(*object);
    jsonFile.write(doc.toJson());
    jsonFile.close();
    return true;
}

QString ShortcutConfigurator::getData(int row,int col)
{
    return shortcutsTable->model()->index(row,col).data().toString();
}

void ShortcutConfigurator::putData(QModelIndex indexA,QString data)
{
    shortcutsTable->model()->setData(indexA, data);
}

void ShortcutConfigurator::raiseError(const QString errorMsg)
{
        QMessageBox messageBox;
        messageBox.critical(0,"Error",errorMsg);
        messageBox.setFixedSize(500,200);
}


void ShortcutConfigurator::updateShortcut(const QModelIndex & indexA, const QModelIndex & indexB)
{
    QString updatedAction = getData(indexA.row(),0);

    auto itr = shortcutsMap.find(updatedAction);
    QAction* changedAction = itr.value();

    QString updatedShortcut = getData(indexA.row(),indexA.column());

    // check if the updated Shortcut is preoccupied or not
    if(shortcutOccupied[updatedShortcut]==true)
    {
        putData(indexA,QString::fromUtf8(""));
        raiseError(QString("Shortcut Already Occupied"));
        return;
    }

    QList<QKeySequence> shortcutsListFinal;

    bool singleShortcutUpdate = false;

    if(indexA.column()==1)
    {
        // get the primary (updatedShortcut)
        shortcutsListFinal.append(updatedShortcut);

        QList<QKeySequence>assignedShortcuts = changedAction->shortcuts();

        if(assignedShortcuts.size()!=0)
        {
            // un-assign the previous primary
            shortcutOccupied.insert(assignedShortcuts[0].toString(QKeySequence::NativeText),false);

            for(int i=1;i<assignedShortcuts.size();i++)
            {
                shortcutsListFinal.append(assignedShortcuts[i]);                    
            }
            changedAction->setShortcuts(shortcutsListFinal);
        }
        else
        {
            changedAction->setShortcut(QKeySequence(updatedShortcut));
            singleShortcutUpdate = true;
        }


    }
    else if(indexA.column()>=1)
    {
   
        QString primaryShortcut = getData(indexA.row(),1);
        if(primaryShortcut!=QString::fromUtf8(""))
        {
            QList<QKeySequence>assignedShortcuts = changedAction->shortcuts();
            if(assignedShortcuts.size()>=indexA.column())
            {
                //sufficent number of columns, replacement
                for(int i=0;i<assignedShortcuts.size();i++)
                {
                    if(i==indexA.column()-1)
                    {
                        shortcutOccupied.insert(assignedShortcuts[i].toString(QKeySequence::NativeText),false);  
                        shortcutsListFinal.append(updatedShortcut);
                    }
                    else shortcutsListFinal.append(assignedShortcuts[i]);                    
                }

            }
            else
            {
                //append the new shortcut
                for(int i=0;i<assignedShortcuts.size();i++)
                {
                    shortcutsListFinal.append(assignedShortcuts[i]);                    
                }
                shortcutsListFinal.append(updatedShortcut);
            }
            changedAction->setShortcuts(shortcutsListFinal);
        }
        else
        {
            putData(indexA,QString::fromUtf8(""));
            raiseError(QString("Primary Shortcut has not been assigned"));
            return;
        }

    }

    if(updatedShortcut!=QString::fromUtf8("")) shortcutOccupied.insert(updatedShortcut,true);

    // write into the file
    QJsonObject object;
    readConfigFile(&object);

    if(singleShortcutUpdate)
    {
        object.insert(updatedAction,updatedShortcut);
    }
    else
    {
        QJsonArray array;
        for (auto & shortcut : shortcutsListFinal) array.append(shortcut.toString());

        QJsonValue newValue = QJsonValue(array);
        object.insert(updatedAction,newValue);            
    }
    writeToConfigFile(&object);

}

void ShortcutConfigurator::on_searchBox_textChanged(const QString &arg1)
{
    QString filterKey=QString("*%1*").arg(arg1);
    QRegExp qr(filterKey,Qt::CaseInsensitive,QRegExp::Wildcard);
    QList<QString>filteredList = actionsName.filter(qr);

    QList<QAction *>newList;
    for(auto entry:filteredList) newList.push_back(shortcutsMap[entry]);

    // regenerate the UI
    initGUI(newList);
    
}

void ShortcutConfigurator::on_reset_clicked()
{   
    QMap<QAction*,QList<QKeySequence>>::iterator i;
    for(i = defaultShortcuts.begin(); i != defaultShortcuts.end(); ++i)
    {
        QAction* actionKey = i.key();
        actionKey->setShortcuts(i.value());
    }
    initGUI(actionsList);
    shortcutOccupied.clear();

    QJsonObject object;
    writeToConfigFile(&object);

}

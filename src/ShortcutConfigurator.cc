#include "ShortcutConfigurator.h"
#include <QFile>
#include <QVariant>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QEvent>


ShortcutConfigurator::ShortcutConfigurator(QWidget *parent): QWidget(parent)
{
    const int numRows = 0;
    const int numColumns = 3;
    model = new QStandardItemModel(numRows, numColumns, shortcutsTable);

    configFileLoc = PlatformUtils::userConfigPath()+"/shortcuts.json";

    setupUi(this);
    connect(shortcutsTable, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableCellClicked(const QModelIndex &)));
}

ShortcutConfigurator::~ShortcutConfigurator()
{
    if(shortcutCatcher)
    {
        delete shortcutCatcher;
    }
    if(model)
    {
        delete model;
    }
}


bool ShortcutConfigurator::eventFilter(QObject *obj, QEvent *event)
{   

    if (event->type() == QEvent::KeyPress)
    { 
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
        
        if(!keyEvent)
        {
            //invalid downcast
            return false;
        } 

        int keyInt = keyEvent->key(); 
        Qt::Key key = static_cast<Qt::Key>(keyInt); 
        if(key == Qt::Key_unknown)
        {
            // Unknown key 
            return false; 
        } 
        
        // the user have clicked just and only the special keys Ctrl, Shift, Alt, Meta. 
        if(key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta)
        { 
            // Single click of special key: Ctrl, Shift, Alt or Meta
            return false; 
        } 

        // check for a combination of user clicks 
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers(); 
        QString keyText = keyEvent->text(); 

        QList<Qt::Key> modifiersList; 
        if(modifiers & Qt::ShiftModifier) 
            keyInt += Qt::SHIFT; 
        if(modifiers & Qt::ControlModifier) 
            keyInt += Qt::CTRL; 
        if(modifiers & Qt::AltModifier) 
            keyInt += Qt::ALT; 
        if(modifiers & Qt::MetaModifier) 
            keyInt += Qt::META; 

        pressedKeySequence = QKeySequence(keyInt);

        if(pressedKeySequence.toString(QKeySequence::NativeText)!=QString(""))
        {
            QString info = QString("You Pressed: %1").arg(pressedKeySequence.toString(QKeySequence::NativeText));
            shortcutCatcher->setInformativeText(info);
            return true;
        }

    }

    return QObject::eventFilter(obj, event);
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

void ShortcutConfigurator::createModel(QObject* parent,const QList<QAction *> &actions)
{
    int row = 0;
    model->removeRows(0,model->rowCount());
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
            actionNameItem->setFlags(actionNameItem->flags() &  ~Qt::ItemIsEditable  & ~Qt::ItemIsSelectable);

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
                shortcutOccupied.insert(shortcut,action);
                index++;
            }
            row++;
        }
    }
}

void ShortcutConfigurator::initGUI(const QList<QAction *> &allActions)
{
    shortcutsTable->verticalHeader()->hide();
    shortcutsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    shortcutsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    shortcutsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    shortcutsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    createModel(shortcutsTable,allActions);
    shortcutsTable->setModel(model);
}

void ShortcutConfigurator::readConfigFile(QJsonObject* object)
{
    QFile jsonFile(QString::fromLocal8Bit(configFileLoc.c_str()));

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
    QFile jsonFile(QString::fromLocal8Bit(configFileLoc.c_str()));

    if (!jsonFile.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QJsonDocument doc(*object);
    jsonFile.write(doc.toJson());
    jsonFile.close();
    return true;
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
                    if(shortcutOccupied.contains(singleShortcut))
                    {
                        raiseError(QString::fromLocal8Bit(configFileLoc.c_str())+QString(":\n"+actionName+" shortcut \""+singleShortcut+"\" conflicts with "+shortcutOccupied[singleShortcut]->objectName()));
                        return;
                    }
                    else if(singleShortcut!=QString()) 
                    {
                        shortcutOccupied.insert(singleShortcut,action);
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
                        if(shortcut==QString()) continue;
                        const QString defaultShortcut = defaultShortcuts[action][0].toString(QKeySequence::NativeText);
                        if (!QString::compare(shortcut, "DEFAULT", Qt::CaseInsensitive))
                        {
                            if(defaultShortcut==QString()) continue;
                            shortcutOccupied.insert(defaultShortcut,action);
                            shortcutsListFinal.append(defaultShortcut);
                        }
                        else 
                        {
                            if(shortcutOccupied.contains(shortcut))
                            {
                                raiseError(QString::fromLocal8Bit(configFileLoc.c_str())+QString(":\n"+actionName+" shortcut \""+shortcut+"\" conflicts with "+shortcutOccupied[shortcut]->objectName()));
                                return;
                            }
                            else if(shortcut!=QString()) 
                            {
                                shortcutOccupied.insert(shortcut,action);
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

void ShortcutConfigurator::onTableCellClicked(const QModelIndex & index)
{
    if (index.isValid() && index.column()!=0) 
    {
        //just to avoid opening of multiple dialogs on multiple clicks on cell
        if(shortcutCatcher) delete shortcutCatcher;

        shortcutCatcher = new QMessageBox;
        QString updatedAction = getData(index.row(),0);
        auto itr = shortcutsMap.find(updatedAction);
        QAction* changedAction = itr.value();

        shortcutCatcher->raise();
        shortcutCatcher->setStyleSheet("QLabel{min-width: 400px;}");
        shortcutCatcher->setInformativeText(QString());
        shortcutCatcher->setWindowTitle(QString("Set Shortcut for: ")+updatedAction);
        shortcutCatcher->installEventFilter(this);
        shortcutCatcher->setText("Press the Key Sequence");
        shortcutCatcher->setStandardButtons(QMessageBox::Apply | QMessageBox::Reset | QMessageBox::Cancel);
        shortcutCatcher->setDefaultButton(QMessageBox::Apply);
        shortcutCatcher->setWindowModality(Qt::WindowModal);
        int ret = shortcutCatcher->exec();
        QString updatedShortcut;

        switch (ret)
        {
            case QMessageBox::Apply:
                updatedShortcut = pressedKeySequence.toString(QKeySequence::NativeText);
                pressedKeySequence = QKeySequence();
                shortcutCatcher->close();
                if(updatedShortcut==QString()) return;
                break;
            case QMessageBox::Reset:
                updatedShortcut = QString();
                shortcutCatcher->close();
                break;
            case QMessageBox::Cancel:
                shortcutCatcher->close();
                return;
        }

        if(shortcutOccupied.contains(updatedShortcut))
        {
            raiseError(QString("Shortcut Already Occupied By: "+shortcutOccupied[updatedShortcut]->objectName()));
            return;
        }

        QList<QKeySequence>assignedShortcuts = changedAction->shortcuts();
        if (index.column() <= assignedShortcuts.size())
        {
            // un-assign the previous primary  
            shortcutOccupied.remove(assignedShortcuts[index.column()-1].toString(QKeySequence::NativeText));
            assignedShortcuts[index.column()-1] = updatedShortcut;
        } else
        {
            // this inserts to the first unused slot, not necessarily corresponding to the clicked column index...
            assignedShortcuts.append(updatedShortcut); 
        }
        changedAction->setShortcuts(assignedShortcuts);

        if(updatedShortcut!=QString()) shortcutOccupied.insert(updatedShortcut,changedAction);
        putData(index,updatedShortcut);
        // write into the file
        QJsonObject object;
        readConfigFile(&object);

        if(assignedShortcuts.size()==1)
        {
            object.insert(updatedAction,updatedShortcut);
        }
        else
        {
            QJsonArray array;
            for (auto & shortcut : assignedShortcuts) array.append(shortcut.toString());

            QJsonValue newValue = QJsonValue(array);
            object.insert(updatedAction,newValue);            
        }
        writeToConfigFile(&object);
    }
}

void ShortcutConfigurator::on_searchBox_textChanged(const QString &arg1)
{
    QString filterKey=QString("*%1*").arg(arg1);
    QRegExp qr(filterKey,Qt::CaseInsensitive,QRegExp::Wildcard);
    QList<QString>filteredList = actionsName.filter(qr);

    QList<QAction *>newList;
    for(auto entry:filteredList) newList.push_back(shortcutsMap[entry]);
    newList.toSet().toList();
    // regenerate the UI
    initGUI(newList);
    
}

void ShortcutConfigurator::on_reset_clicked()
{   
    QMap<QAction*,QList<QKeySequence>>::iterator i;
    QJsonObject object;

    QMessageBox msgBox;
    msgBox.setText("Choosing 'Yes' will restore all the shortcuts!");
    msgBox.setInformativeText("Do you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();

    switch (ret)
    {
        case QMessageBox::Yes:
            for(i = defaultShortcuts.begin(); i != defaultShortcuts.end(); ++i)
            {
                QAction* actionKey = i.key();
                actionKey->setShortcuts(i.value());
            }
            initGUI(actionsList);
            shortcutOccupied.clear();
            writeToConfigFile(&object);
            break;
        case QMessageBox::No:
            break;
        default:
            break;
    }
}

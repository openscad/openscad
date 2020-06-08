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
#include <QMessageBox>

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
    QList<QString> labels = QList<QString>() << QString("Action") << QString("Shortcut")<<QString("Alternative-1"); 
    model->setHorizontalHeaderLabels(labels);
    for (auto &action : actions) 
    {
        QString actionName = action->objectName();
        if(!actionName.isEmpty())
        {
        QStandardItem* actionNameItem = new QStandardItem(actionName);
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

            shortcutOccupied.insert(shortcut,true);

        }

        if(!this->shortcutsMap.contains(actionName)) {
            this->actionsName.push_back(actionName);
            this->shortcutsMap.insert(actionName,action);
            }
        row++;
        }
    }
    return model;
}




void ShortcutConfigurator::initGUI(const QList<QAction *> &allActions)
{
    this->shortcutsTable->verticalHeader()->hide();
    this->shortcutsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    initTable(this->shortcutsTable,allActions);
    connect(this->shortcutsTable->model(),SIGNAL(dataChanged(QModelIndex,QModelIndex)),SLOT(updateShortcut(QModelIndex,QModelIndex)));
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

void ShortcutConfigurator::putData(QModelIndex indexA,QString data)
{
    QTableView *shortcutTableView = this->findChild<QTableView *>();
    shortcutTableView->model()->setData(indexA, data);

}

void ShortcutConfigurator::readConfigFile(QJsonObject* object)
{
    std::string absolutePath = PlatformUtils::userConfigPath()+"/shortcuts.json";

	QString finalPath = QString::fromLocal8Bit(absolutePath.c_str());

	QFile jsonFile(finalPath);
	
    //check if a User-Defined Shortcuts file exists or not
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


void ShortcutConfigurator::updateShortcut(const QModelIndex & indexA, const QModelIndex & indexB)
{
    QString updatedAction = getData(indexA.row(),0);

    auto itr = this->shortcutsMap.find(updatedAction);
    QAction* changedAction = itr.value();

    QString updatedShortcut = getData(indexA.row(),indexA.column());

    // handle when shortcut is unassigned-todo

    //check if the updated Shortcut is preoccupied or not
    if(shortcutOccupied[updatedShortcut])
    {
        putData(indexA,QString::fromUtf8(""));
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Shortcut is already assigned to some other action");
        messageBox.setFixedSize(500,200);
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
        if(!primaryShortcut.isEmpty())
        {
            QList<QKeySequence>assignedShortcuts = changedAction->shortcuts();
            if(assignedShortcuts.size()>=indexA.column())
            {
                //sufficent number of columns, replacement
                for(int i=0;i<assignedShortcuts.size();i++)
                {
                    if(i==indexA.column()-1) {shortcutsListFinal.append(updatedShortcut);}
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

    }

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
    QString filterKey=QString::fromUtf8("");
    filterKey.append("*");
    filterKey.append(arg1);
    filterKey.append("*");
    QRegExp qr(filterKey,Qt::CaseInsensitive,QRegExp::Wildcard);
    QList<QString>filteredList = this->actionsName.filter(qr);

    QList<QAction *>newList;
    for(auto entry:filteredList) newList.push_back(shortcutsMap[entry]);

    //regenerate the UI
    this->initGUI(newList);
    
}

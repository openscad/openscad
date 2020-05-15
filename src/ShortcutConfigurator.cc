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

ShortcutConfigurator::ShortcutConfigurator(QWidget *parent): QWidget(parent)
{
    setupUi(this);
}

ShortcutConfigurator::~ShortcutConfigurator()
{
    
}


void ShortcutConfigurator::collectActions(const QList<QAction *> &actions)
{
    actionList = actions;
}

QStandardItemModel* ShortcutConfigurator::createModel(QObject* parent)
{
        

    const int numRows = 10;
    const int numColumns = 10;

    QStandardItemModel* model = new QStandardItemModel(numRows, numColumns);
    for (int row = 0; row < numRows; ++row)
    {
        for (int column = 0; column < numColumns; ++column)
        {
            QString text = QString('A' + row) + QString::number(column + 1);
            QStandardItem* item = new QStandardItem(text);
            model->setItem(row, column, item);
        }
     }

    return model;
}

void ShortcutConfigurator::initGUI()
{
    
    QTableView *shortcutTableView = this->findChild<QTableView *>();
    // std::cout<<table->objectName().toStdString()<<" initGUI"<<std::endl;
    initTable(shortcutTableView);
    // table->setModel(createModel(table));


}

void ShortcutConfigurator::initTable(QTableView *shortcutsTable)
{
    
    shortcutsTable->setModel(createModel(shortcutsTable));

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

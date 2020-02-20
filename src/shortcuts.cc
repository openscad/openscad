#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QVariant>
#include "shortcuts.h"
#include "PlatformUtils.h"
#include "printutils.h"
#include "qtgettext.h"

ShortCutConfigurator::ShortCutConfigurator(){


}

void ShortCutConfigurator::apply(const QList<QAction *> &actions)
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
	QJsonObject jObject = doc.object();
	QVariantMap json_map = jObject.toVariantMap();

    for (auto &action : actions) 
    {
            
        QString actionName = action->objectName();
 
        QMap<QString, QVariant>::const_iterator i = json_map.find(actionName);
        //check if the actions new shortcut exists in the file or not
        //This would allow users to remove any default shortcut, by just leaving the key's value for an action name as empty.
        if(i != json_map.end() && i.key() == actionName)
        {
            QString shortcut =  json_map[actionName].toString().trimmed();
            action->setShortcut(QKeySequence(shortcut));
        }

    }

}

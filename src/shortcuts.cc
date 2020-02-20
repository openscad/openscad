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
        const QString shortCut(action->shortcut().toString(QKeySequence::NativeText));
        //handle the case, if shortcut is not defined for an action
        if (shortCut.isEmpty())
        {
        continue;
        }
    
        QString actionName = action->objectName();
        QString shortcut =  json_map[actionName].toString();
        if(!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
        }
    }

}

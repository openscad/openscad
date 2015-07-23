#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QWidget>
#include <dirent.h>
#include <string>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
class Toolbar
{

	public:
	Toolbar();
	virtual ~Toolbar(){ };
	QStringList iconSchemes();

};

#include "toolbar.h"
#include <iostream>
#include "PlatformUtils.h"

Toolbar::Toolbar()
{ }

QStringList Toolbar::iconSchemes()
{
	QStringList iconSchemes;
	fs::path base_path = PlatformUtils::resourceBasePath();
	fs::path path = base_path / "images";
	DIR *dir = opendir(boosty::stringy(path).c_str());

	struct dirent *entry = readdir(dir);

	while (entry != NULL)
	{
		if(entry->d_type == DT_DIR){
			std::string name = entry->d_name;
			if((name == ".") || (name == "..")){
			} else {
				QString name_ = QString::fromStdString(name);
				iconSchemes << name_;
			}
		}
		entry = readdir(dir);
	}
	return iconSchemes;

}


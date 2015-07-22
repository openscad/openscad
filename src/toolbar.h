#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QWidget>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
class ToolbarIconSet
{

private:
        const fs::path path;
        
        boost::property_tree::ptree pt;
        QString _name;
        int _index;
        
public:
        ToolbarIconSet(const fs::path path);
        virtual ~ToolbarIconSet();
        
        const QString & name() const;
        int index() const;
        bool valid() const;
        const boost::property_tree::ptree & propertyTree() const;
        
};

class Toolbar
{

	public:
	Toolbar();
	virtual ~Toolbar(){ };
        typedef std::multimap<int, boost::shared_ptr<ToolbarIconSet>, std::less<int> > iconscheme_set_t;

	QStringList iconSchemes();

	private:	
        void enumerateColorSchemesInPath(iconscheme_set_t &result_set, const fs::path path);
        iconscheme_set_t enumerateColorSchemes();
};

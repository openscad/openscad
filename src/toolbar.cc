#include "toolbar.h"
#include <iostream>
#include "PlatformUtils.h"
ToolbarIconSet::ToolbarIconSet(fs::path path) : path(path)
{
	std::cout << "calling contructor" << std::endl;
	try {	
		const std::string &name = boosty::stringy(path).c_str();
		pt.put("name", name);
		_name = QString(pt.get<std::string>("name").c_str());
		//_index = pt.get<int>("index");
	} catch (const std::exception & e) {
		PRINTB("Error reading icon set '%s': %s", boosty::stringy(path).c_str() % e.what());
		_name = "";
		_index = 0;
	}
}

ToolbarIconSet::~ToolbarIconSet()
{

}

bool ToolbarIconSet::valid() const
{
	return !_name.isEmpty();
}

const QString & ToolbarIconSet::name() const
{
	return _name;
}

int ToolbarIconSet::index() const
{
	return _index;
}

const boost::property_tree::ptree & ToolbarIconSet::propertyTree() const
{
	return pt;
}

Toolbar::Toolbar()
{

}
void Toolbar::enumerateColorSchemesInPath(Toolbar::iconscheme_set_t &result_set, const fs::path path)
{
	const fs::path color_schemes = path / "images";

	fs::directory_iterator end_iter;

	if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
		std::cout <<"color scheme exists";
		for (fs::directory_iterator dir_iter(color_schemes); dir_iter != end_iter; ++dir_iter) {
			if (!fs::is_directory(dir_iter->status())) {
				continue;
			}

			const fs::path path = (*dir_iter).path();

			ToolbarIconSet *iconset = new ToolbarIconSet(path);
			if (iconset->valid()) {
				result_set.insert(iconscheme_set_t::value_type(iconset->index(), boost::shared_ptr<ToolbarIconSet>(iconset)));
			} else {
				delete iconset;
			}
		}
	}
}

Toolbar::iconscheme_set_t Toolbar::enumerateColorSchemes()
{
	iconscheme_set_t result_set;

	enumerateColorSchemesInPath(result_set, PlatformUtils::resourceBasePath());
	enumerateColorSchemesInPath(result_set, PlatformUtils::userConfigPath());

	return result_set;
}

QStringList Toolbar::iconSchemes()
{
	const iconscheme_set_t iconscheme_set = enumerateColorSchemes();
	QStringList iconSchemes;
	for (iconscheme_set_t::const_iterator it = iconscheme_set.begin(); it != iconscheme_set.end(); it++) {
		iconSchemes << (*it).second.get()->name();
	}

	return iconSchemes;
}


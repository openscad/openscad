#include <stdio.h>
#include <iostream>
#include <string>
#include <map>

#include "feature.h"

/**
 * Feature registration map for later lookup. This must be initialized
 * before the static feature instances as those register with this map. 
 */
std::map<std::string, Feature *> Feature::feature_map;

/*
 * List of features, the names given here are used in both command line
 * argument to enable the option and for saving the option value in GUI
 * context.
 */
const Feature Feature::ExperimentalConcatFunction("concat");

Feature::Feature(std::string name) : enabled_cmdline(false), enabled_options(false), name(name)
{
        feature_map[name] = this;
}

Feature::~Feature()
{
}

const std::string& Feature::get_name() const
{
        return name;
}
    
void Feature::set_enable_cmdline()
{
	enabled_cmdline = true;
}

void Feature::set_enable_options(bool status)
{
	enabled_options = status;
}

bool Feature::is_enabled() const
{
	if (enabled_cmdline) {
	    return true;
	}
	return enabled_options;
}
    
bool operator ==(const Feature& lhs, const Feature& rhs)
{
        return lhs.get_name() == rhs.get_name();
}

bool operator !=(const Feature& lhs, const Feature& rhs)
{
        return !(lhs == rhs);
}
    
void Feature::enable_feature(std::string feature_name)
{
	map_t::iterator it = feature_map.find(feature_name);
	if (it != feature_map.end()) {
		(*it).second->set_enable_cmdline();
	}
}

void Feature::enable_feature(std::string feature_name, bool status)
{
	map_t::iterator it = feature_map.find(feature_name);
	if (it != feature_map.end()) {
		(*it).second->set_enable_options(status);
	}
}

void Feature::dump_features()
{
	for (map_t::iterator it = feature_map.begin(); it != feature_map.end(); it++) {
		std::cout << "Feature('" << (*it).first << "') = " << ((*it).second->is_enabled() ? "enabled" : "disabled") << std::endl;
	}
}

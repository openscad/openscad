#include <stdio.h>
#include <iostream>
#include <string>
#include <map>

#include "feature.h"
#include "printutils.h"

/**
 * Feature registration map/list for later lookup. This must be initialized
 * before the static feature instances as those register with this map. 
 */
Feature::map_t Feature::feature_map;
Feature::list_t Feature::feature_list;

/*
 * List of features, the names given here are used in both command line
 * argument to enable the option and for saving the option value in GUI
 * context.
 */
const Feature Feature::ExperimentalConcatFunction("experimental/concat-function", "Enable the <code>concat()</code> function.");

Feature::Feature(const std::string name, const std::string description) : enabled_cmdline(false), enabled_options(false), name(name), description(description)
{
        feature_map[name] = this;
	feature_list.push_back(this);
}

Feature::~Feature()
{
}

const std::string& Feature::get_name() const
{
        return name;
}
    
const std::string& Feature::get_description() const
{
        return description;
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

void Feature::enable(bool status)
{
	set_enable_options(status);
}
    
void Feature::enable_feature(std::string feature_name)
{
	map_t::iterator it = feature_map.find(feature_name);
	if (it != feature_map.end()) {
		(*it).second->set_enable_cmdline();
	} else {
		PRINTB("WARNING: Ignoring request to enable unknown feature '%s'.", feature_name);
	}
}

void Feature::enable_feature(std::string feature_name, bool status)
{
	map_t::iterator it = feature_map.find(feature_name);
	if (it != feature_map.end()) {
		(*it).second->set_enable_options(status);
	}
}

Feature::iterator Feature::begin()
{	
	return feature_list.begin();
}

Feature::iterator Feature::end()
{
	return feature_list.end();
}

void Feature::dump_features()
{
	for (map_t::iterator it = feature_map.begin(); it != feature_map.end(); it++) {
		std::cout << "Feature('" << (*it).first << "') = " << ((*it).second->is_enabled() ? "enabled" : "disabled") << std::endl;
	}
}

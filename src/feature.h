#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

class Feature
{
public:
	typedef std::vector<Feature *> list_t;
	typedef list_t::iterator iterator;

	const std::string& get_name() const;
	const std::string& get_description() const;
    
	bool is_enabled() const;
	void enable(bool status);

	static iterator begin();
	static iterator end();
    
	static void dump_features();
	static void enable_feature(const std::string &feature_name, bool status = true);

private:
	bool enabled;
  
	const std::string name;
	const std::string description;
    
	typedef std::map<std::string, Feature *> map_t;
	static map_t feature_map;
	static list_t feature_list;
    
	Feature(const std::string &name, const std::string &description);
	virtual ~Feature();
};

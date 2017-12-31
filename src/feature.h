#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "exceptions.h"

class Feature
{
public:
	typedef std::vector<Feature *> list_t;
	typedef list_t::iterator iterator;

	static const Feature ExperimentalAssertExpression;
	static const Feature ExperimentalEchoExpression;
        static const Feature ExperimentalEachExpression;
        static const Feature ExperimentalElseExpression;
        static const Feature ExperimentalForCExpression;
        static const Feature ExperimentalAmfImport;
        static const Feature ExperimentalSvgImport;
        static const Feature ExperimentalCustomizer;


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

class ExperimentalFeatureException : public EvaluationException
{
public:
	static void check(const Feature &feature);
	~ExperimentalFeatureException() throw();

private:
	ExperimentalFeatureException(const std::string &what_arg);
};

#ifndef FEATURE_H_
#define FEATURE_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

class Feature {
public:
    typedef std::vector<Feature *> list_t;
    typedef list_t::iterator iterator;

private:
    /**
     * Set to true in case the matching feature was given as commandline
     * argument.
     */
    bool enabled_cmdline;
    /**
     * Set from the GUI options. This will not be set in case the GUI is
     * not started at all.
     */
    bool enabled_options;
    
    const std::string name;
    const std::string description;
    
    typedef std::map<std::string, Feature *> map_t;
    static map_t feature_map;
    static list_t feature_list;
    
    Feature(std::string name, std::string description);
    virtual ~Feature();
    virtual void set_enable_cmdline();
    virtual void set_enable_options(bool status);

public:
    static const Feature ExperimentalConcatFunction;
    
    const std::string& get_name() const;
    const std::string& get_description() const;
    
    bool is_enabled() const;
    void enable(bool status);

    static iterator begin();
    static iterator end();
    
    static void dump_features();
    static void enable_feature(std::string feature_name);
    static void enable_feature(std::string feature_name, bool status);
};

#endif

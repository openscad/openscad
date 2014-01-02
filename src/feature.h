#ifndef FEATURE_H_
#define FEATURE_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>

class Feature {
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
    
    std::string name;
    
    typedef std::map<std::string, Feature *> map_t;
    static map_t feature_map;

    Feature(std::string name);
    virtual ~Feature();
    virtual void set_enable_cmdline();
    virtual void set_enable_options(bool status);

public:
    static const Feature ExperimentalConcatFunction;
    
    const std::string& get_name() const;
    
    bool is_enabled() const;
    
    friend bool operator ==(const Feature& lhs, const Feature& rhs);
    friend bool operator !=(const Feature& lhs, const Feature& rhs);
    
    static void dump_features();
    static void enable_feature(std::string feature_name);
    static void enable_feature(std::string feature_name, bool status);
};

#endif

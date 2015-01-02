#pragma once

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <boost/shared_ptr.hpp>

#include "value.h"

class Annotation;

typedef std::map<const std::string, Annotation *> AnnotationMap;

typedef std::vector<Annotation> AnnotationList;

class Assignment : public std::pair<std::string, boost::shared_ptr<class Expression> >
{
public:
    Assignment(std::string name);
    Assignment(std::string name, boost::shared_ptr<class Expression> expr);
    virtual ~Assignment();

    virtual void add_annotations(AnnotationList *annotations);
    
    virtual bool has_annotations() const;
    virtual const Annotation * annotation(const std::string &name) const;

protected:
    AnnotationMap annotations;
};

typedef std::vector<Assignment> AssignmentList;
typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;

class Annotation
{
protected:
    Annotation(const std::string name, const AssignmentList assignments, const AssignmentList args);
    
public:
    virtual ~Annotation();

    void dump();
    const std::string & get_name();
    virtual ValuePtr evaluate(class Context *ctx, const std::string& var) const;
    
    Annotation& operator=(const Annotation&);

    static const Annotation * create(const std::string name, const AssignmentList assignments);

//private:
    std::string name;
    /** Defines the names and values parsed from the script */
    AssignmentList assignments;
private:
    /** Defines the names of the positional parameters */
    AssignmentList args;
};


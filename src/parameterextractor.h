#ifndef PARAMETEREXTRACTOR_H
#define PARAMETEREXTRACTOR_H

#include "qtgettext.h"
#include "parameterobject.h"
#include "parametervirtualwidget.h"

class ParameterExtractor
{

protected:

    typedef std::map<std::string, class ParameterObject *> entry_map_t;
    entry_map_t entries;

public:
    ParameterExtractor();
    virtual ~ParameterExtractor();
    void setParameters(const Module *module);
    void applyParameters(class FileModule *fileModule);


protected:

    virtual void begin()=0;
    virtual void addEntry(ParameterVirtualWidget *entry)=0;
    virtual void end()=0;
    virtual void connectWidget()=0;
};

#endif // PARAMETEREXTRACTOR_H

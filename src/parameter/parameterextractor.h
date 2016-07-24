#ifndef PARAMETEREXTRACTOR_H
#define PARAMETEREXTRACTOR_H

#include "FileModule.h"
#include "parametervirtualwidget.h"

class ParameterExtractor
{

protected:

    typedef std::map<std::string, class ParameterObject *> entry_map_t;
    entry_map_t entries;

public:
    ParameterExtractor();
    virtual ~ParameterExtractor();
    void setParameters(const FileModule* module);
    void applyParameters(FileModule *fileModule);


protected:
    virtual void begin()=0;
    virtual void addEntry(ParameterVirtualWidget *entry)=0;
    virtual void end()=0;
    virtual void connectWidget()=0;
};

#endif // PARAMETEREXTRACTOR_H

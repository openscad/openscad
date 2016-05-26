#ifndef EXTRACTOR_H
#define EXTRACTOR_H



#include "qtgettext.h"
#include "parameterobject.h"
class Extractor
{
public:
    Extractor();
    typedef std::map<std::string, class ParameterObject *> entry_map_t;
    entry_map_t entries;
    void setParameters(const Module *module);
    void applyParameters(class FileModule *fileModule);
    virtual void begin()=0;
    virtual void addEntry(class ParameterEntryWidget *entry)=0;
    virtual void end()=0;
    virtual void connectWidget(ParameterObject *param)=0;
};

#endif // EXTRACTOR_H

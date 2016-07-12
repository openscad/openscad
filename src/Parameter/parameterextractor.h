#ifndef PARAMETEREXTRACTOR_H
#define PARAMETEREXTRACTOR_H

#include "FileModule.h"
#include "parametervirtualwidget.h"
#include<vector>
#include<QJsonValue>
class ParameterExtractor
{

protected:

    typedef map<string,QJsonValue> SetOfParameter;
    typedef map<string,SetOfParameter> ParameterSet;
     ParameterSet parameterSet;

    typedef std::map<std::string, class ParameterObject *> entry_map_t;
    entry_map_t entries;

public:
    ParameterExtractor();
    virtual ~ParameterExtractor();
    void setParameters(const FileModule* module);
    void applyParameters(class FileModule *fileModule);
    bool getParameterSet(string filename);
    void print();
    void applyParameterSet(FileModule *fileModule,string setName);

protected:
    virtual void begin()=0;
    virtual void addEntry(ParameterVirtualWidget *entry)=0;
    virtual void end()=0;
    virtual void connectWidget()=0;
};

#endif // PARAMETEREXTRACTOR_H
